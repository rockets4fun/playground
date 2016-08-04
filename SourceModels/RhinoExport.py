import sys
import os

import Rhino as rc
import rhinoscriptsyntax as rs
import scriptcontext as sc

import struct
import System

typeStr = \
{
            0 : "unknown",
            1 : "point",
            2 : "point cloud",
            4 : "curve",
            8 : "surface",
           16 : "polysurface",
           32 : "mesh",
          256 : "light",
          512 : "annotation",
         4096 : "instance",
         8192 : "text dot object",
        16384 : "grip object",
        32768 : "detail",
        65536 : "hatch",
       131072 : "morph control",
    134217728 : "cage",
    268435456 : "phantom",
    536870912 : "clipping plane",
}

g_meshParams = rc.Geometry.MeshingParameters.Smooth

g_instances = []
g_instancesByName = {}
g_parts = []

g_materials = {}

g_indent = "    "

def formatHexFloats(floats, comment=False) :
    result = ""

    for float in floats :
        part = hex(struct.unpack("<I", struct.pack("<f", float))[0])
        part = part.lstrip("0x")
        part = part.rstrip("L")
        part = part.rjust(8, "0")
        if len(result) :
            part = " " + part
        result += part

    if comment :
        result += " #";
        for float in floats :
            result += format("%6.2f" % float)

    return result

def formatColors(colors) :
    result = ""

    for color in colors :
        part = format("%08x" % (2**32 + color.ToArgb()))
        part = part[2:] + part[0:2]
        if len(result) :
            part = " " + part
        result += part

    return result

def processObject(object, parentInstances) :
    global g_instances
    global g_instancesByName
    global g_parts
    global g_materials

    name = rs.ObjectName(object)
    type = rs.ObjectType(object)
    layer = rs.ObjectLayer(object)

    if type == rs.filter.instance :
        type = rs.BlockInstanceName(object)
        xform = rs.BlockInstanceXform(object)
        subObjects = rs.ExplodeBlockInstance(object)

        fullName = name
        if len(parentInstances) > 1 :
            for parent in parentInstances[1:] :
                fullName = parent["name"] + "." + fullName
        originalFullName = fullName

        appendixCtr = 1
        while fullName in g_instancesByName :
            fullName = format("%s+%d" % (originalFullName, appendixCtr))
            appendixCtr += 1
        if fullName != originalFullName :
            print("WARNING: Renamed %s => %s" %
                (originalFullName, fullName))

        instance = \
        {
               "name" : name,
           "fullName" : fullName,
               "type" : type,
              "xform" : xform,
            "parents" : list(parentInstances),
              "parts" : [],
            "touched" : False,
        }
        g_instances.append(instance)
        g_instancesByName[fullName] = instance

        for subObject in subObjects :
            processObject(subObject, parentInstances + [instance])
        return

    skipReason = None
    if rs.LayerLocked(layer) :
        skipReason = "layer locked"
    elif not rs.LayerVisible(layer) :
        skipReason = "layer hidden"
    elif type != rs.filter.polysurface and type != rs.filter.surface :
        skipReason = "bad type - " + typeStr[type]
    elif not name :
        skipReason = "no name"

    if skipReason :
        # make sure we can delete object by moving to current layer
        rs.ObjectLayer(object, rs.CurrentLayer())
        print("Skipping %s (%s)" % (str(object), skipReason))
    else :
        brep = rs.coercebrep(object)
        meshes = rc.Geometry.Mesh.CreateFromBrep(brep, g_meshParams)
        joinedMesh = rc.Geometry.Mesh()
        for mesh in meshes :
            joinedMesh.Append(mesh)
        joinedMesh.Reduce(0, False, 10, False)
        if not joinedMesh.Faces.ConvertQuadsToTriangles() :
            print("WARNING: Failed to convert quads to tris for %s" % (str(object)))
        if not joinedMesh.Compact() :
            print("WARNING: Failed to compact %s" % (str(object)))

        materialSrc = rs.ObjectMaterialSource(object)
        if materialSrc == 0 :
            materialIdx = rs.LayerMaterialIndex(rs.ObjectLayer(object))
        else :
            materialIdx = rs.ObjectMaterialIndex(object)

        material = rs.MaterialName(materialIdx)
        g_materials[material] = materialIdx

        joinedMeshGuid = sc.doc.Objects.AddMesh(joinedMesh)
        rs.ObjectName(joinedMeshGuid, name)
        rs.ObjectMaterialSource(joinedMeshGuid, 1)
        rs.ObjectMaterialIndex(joinedMeshGuid, materialIdx)

        part = \
        {
                "name" : name,
                "mesh" : joinedMesh,
            "instance" : parentInstances[-1],
            "material" : material,
        }
        parentInstances[-1]["parts"].append(part)
        if not parentInstances[-1]["touched"] :
            for parentInstance in parentInstances :
                parentInstance["touched"] = True
        g_parts.append(part)

    rs.DeleteObject(object)

def main() :
    global g_instances
    global g_parts
    global g_indent
    global g_materials

    #print(sys.version_info) # (2, 7, 0, 'beta', 0)

    dlg = rc.UI.SaveFileDialog()
    dlg.DefaultExt = "model"
    dlg.Filter = "RocketScience 3D Model (*.model)"
    dlg.InitialDirectory = os.path.dirname(sc.doc.Path)
    if not dlg.ShowSaveDialog() : return None

    selectedObjects = rs.SelectedObjects()

    objects = []
    origin = [0,0,0]

    for object in selectedObjects :
        name = rs.ObjectName(object)
        type = rs.ObjectType(object)
        if type == rs.filter.point :
            if name == "Origin" :
                origin = rs.PointCoordinates(object)
        else :
            objects.append(object)

    print("Processing " + str(len(objects)) + " object(s)")
    print("Origin is [%.2f,%.2f,%.2f]" % (origin[0],origin[1],origin[2]))

    rootInstance = \
    {
            "name" : "*",
        "fullName" : "*",
            "type" : "*",
           "xform" : None,
         "parents" : [],
           "parts" : [],
         "touched" : False,
    }

    g_instances.append(rootInstance)

    objectsCopied = rs.CopyObjects(objects, rs.VectorScale(origin, -1))
    for object in objectsCopied :
        processObject(object, [rootInstance])

    output = open(dlg.FileName, "w")

    #output.write("# i <instance-type> <instance-name> "
    #    "<parent-instance-name>\n")
    #output.write("# x <x-axis-x> <y-axis-x> <z-axis-x> <translation-x>\n")
    #output.write("# x <x-axis-y> <y-axis-y> <z-axis-y> <translation-y>\n")
    #output.write("# x <x-axis-z> <y-axis-z> <z-axis-z> <translation-z>\n")
    #output.write("# m <material-name> <ambient> <diffuse> <emission>\n")
    #output.write("# p <part-name> <instance-name> <material-name> "
    #    "<vertices> <triangles>\n")
    #output.write("# v <pos-x> <pos-y> <pox-z> <norm-x> <norm-y> <norm-z>\n")
    #output.write("# t <vertex-1> <vertex-2> <vertex-3>\n")
    #output.write("\n")

    for instance in g_instances :
        parentCount = len(instance["parents"])

        indent = g_indent * (parentCount - 1)

        parentName = "*"
        if instance["parents"] :
            parentName = instance["parents"][-1]["name"]
        line = format("i%s %s %s %s" % (indent,
            instance["type"], instance["fullName"], parentName))

        print(line)
        output.write(line + "\n")

        if parentCount < 1 :
            xform = rc.Geometry.Transform(1.0)
        else :
            xform = instance["xform"]

        indent += "  "
        output.write("x " + indent + formatHexFloats(
            [xform.M00, xform.M01, xform.M02, xform.M03], True) + "\n")
        output.write("x " + indent + formatHexFloats(
            [xform.M10, xform.M11, xform.M12, xform.M13], True) + "\n")
        output.write("x " + indent + formatHexFloats(
            [xform.M20, xform.M21, xform.M22, xform.M23], True) + "\n")

    for materialName, materialIdx in g_materials.iteritems() :
        material = sc.doc.Materials[materialIdx]

        ambient = material.AmbientColor;
        diffuse = material.DiffuseColor;
        emission = material.EmissionColor;

        line = format("m %s %s" % (materialName,
            formatColors([ambient, diffuse, emission])))

        print(line)
        output.write(line + "\n")

    for part in g_parts :
        mesh = part["mesh"]

        line = format("p %s %s %s %d %d" % (
            part["name"], part["instance"]["fullName"], part["material"],
            mesh.Vertices.Count, mesh.Faces.Count))

        print(line)
        output.write(line + "\n")

        indent = "  "

        for vertexIdx in range(0, mesh.Vertices.Count) :
            vertex = mesh.Vertices[vertexIdx]
            normal = mesh.Normals[vertexIdx]
            output.write("v " + indent + formatHexFloats([
                vertex.X, vertex.Y, vertex.Z,
                normal.X, normal.Y, normal.Z]) + "\n")

        line = lineInit = "t" + indent
        for faceIdx in range(0, mesh.Faces.Count) :
            face = mesh.Faces[faceIdx]
            if not face.IsTriangle :
                print("WARNING: Non-triangle face %d" % (faceIdx))
            part = format(" %d %d %d" % (face.A, face.B, face.C))
            if len(line) + len(part) > 100 :
                output.write(line + "\n")
                line = lineInit
            line += part
        if len(line) > 0 :
            output.write(line + "\n")

    output.close()

main()
