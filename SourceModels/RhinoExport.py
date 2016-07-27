import sys
import os

import Rhino as rc
import rhinoscriptsyntax as rs
import scriptcontext as sc
import struct

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
g_parts = []

g_indent = "    "

def floatsToHex(floats) :
    result = ""
    for float in floats :
        part = hex(struct.unpack("<I", struct.pack("<f", float))[0])
        part = part.lstrip("0x")
        part = part.rstrip("L")
        part = part.rjust(8, "0")
        if len(result) :
            part = " " + part
        result += part
    return result

def formatHexFloats(floats, comment=False) :
    result = ""
    result += floatsToHex(floats)
    if comment :
        result += " #";
        for float in floats :
            result += format("%6.2f" % float)
    return result

def processObject(object, parentInstances) :
    global g_parts

    name = rs.ObjectName(object)
    type = rs.ObjectType(object)
    layer = rs.ObjectLayer(object)

    if type == rs.filter.instance :
        type = rs.BlockInstanceName(object)
        xform = rs.BlockInstanceXform(object)
        subObjects = rs.ExplodeBlockInstance(object)

        instance = \
        {
               "name" : name,
               "type" : type,
              "xform" : xform,
            "parents" : list(parentInstances),
              "parts" : [],
        }
        global g_instances
        g_instances.append(instance)

        for subObject in subObjects :
            processObject(subObject, parentInstances + [instance])
        return

    skipReason = None
    if rs.LayerLocked(layer) :
        skipReason = "layer locked"
    elif not rs.LayerVisible(layer) :
        skipReason = "layer hidden"
    elif type != rs.filter.polysurface :
        skipReason = "bad type - " + typeStr[type]
    elif not name :
        skipReason = "no name"

    if skipReason :
        # make sure we can delete object by moving to current layer
        rs.ObjectLayer(object, rs.CurrentLayer())
        print("WARNING: Skipping %s (%s)" % (str(object), skipReason))
    elif type == rs.filter.polysurface :
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

        joinedMeshGuid = sc.doc.Objects.AddMesh(joinedMesh)
        rs.ObjectName(joinedMeshGuid, name)
        rs.ObjectMaterialSource(joinedMeshGuid, 1)
        rs.ObjectMaterialIndex(joinedMeshGuid, rs.LayerMaterialIndex(layer))

        part = \
        {
                "name" : name,
                "mesh" : joinedMesh,
            "instance" : parentInstances[-1],
        }
        parentInstances[-1]["parts"].append(part)
        g_parts.append(part)

    rs.DeleteObject(object)

def main() :
    global g_instances
    global g_parts
    global g_indent

    #print(sys.version_info) # (2, 7, 0, 'beta', 0)

    dlg = rc.UI.SaveFileDialog()
    dlg.DefaultExt = "model"
    dlg.Filter = "RocketScience 3D Model (*.model)"
    dlg.InitialDirectory = os.path.dirname(sc.doc.Path)
    if not dlg.ShowSaveDialog() : return None

    output = open(dlg.FileName, "w")

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
                 "name" : "Root",
                 "type" : "None",
        "blockInstance" : None,
              "parents" : [],
                "parts" : [],
    }

    g_instances.append(rootInstance)

    objectsCopied = rs.CopyObjects(objects, rs.VectorScale(origin, -1))
    for object in objectsCopied :
        processObject(object, [rootInstance])

    for instance in g_instances :
        parentCount = len(instance["parents"])

        indent = g_indent * (parentCount - 1)
        line = ("i " + indent
            + instance["type"] + " " + instance["name"])
        output.write(line + "\n")
        print(line)

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

    for part in g_parts :
        mesh = part["mesh"]

        line = ("p " + part["name"]
            + " " + str(mesh.Vertices.Count)
            + " " + str(mesh.Faces.Count))

        output.write(line + "\n")
        print(line)

        indent = "  "

        for vertexIdx in range(0, mesh.Vertices.Count) :
            vertex = mesh.Vertices[vertexIdx]
            normal = mesh.Normals[vertexIdx]
            output.write("v " + indent + formatHexFloats([
                vertex.X, vertex.Y, vertex.Z,
                normal.X, normal.Y, normal.Z]) + "\n")

        line = ""
        for faceIdx in range(0, mesh.Faces.Count) :
            if not line :
                line = "t " + indent
            else :
                line += " "
            face = mesh.Faces[faceIdx]
            if not face.IsTriangle :
                print("WARNING: Non-triangle face %d" % (faceIdx))
            line += format("%d %d %d" % (face.A, face.B, face.C))
            if len(line) > 50 :
                output.write(line + "\n")
                line = ""
        if len(line) > 0 :
            output.write(line + "\n")

    output.close()

main()
