import sys

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

def floatsToHex(floats):
    result = ""
    for float in floats:
        part = hex(struct.unpack("<I", struct.pack("<f", float))[0])
        part = part.lstrip("0x")
        part = part.rstrip("L")
        part = part.ljust(8, "0")
        result += part + " "
    return result

def formatHexFloats(floats):
    result = ""
    result += floatsToHex(floats)
    result += "# [";
    for float in floats:
        result += format("%6.2f" % float)
    result += " ]";
    return result

def processObject(object, parentInstances):
    global g_parts

    name = rs.ObjectName(object)
    type = rs.ObjectType(object)
    layer = rs.ObjectLayer(object)

    if type == rs.filter.instance:
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

        for subObject in subObjects:
            processObject(subObject, parentInstances + [instance])
        return

    skipReason = None
    if rs.LayerLocked(layer):
        skipReason = "layer locked"
    elif not rs.LayerVisible(layer):
        skipReason = "layer hidden"
    elif type != rs.filter.polysurface:
        skipReason = "bad type - " + typeStr[type]
    elif not name:
        skipReason = "no name"

    if skipReason :
        # make sure we can delete object by moving to current layer
        rs.ObjectLayer(object, rs.CurrentLayer())
        print("WARNING: Skipping %s (%s)" % (str(object), skipReason))
    elif type == rs.filter.polysurface:
        brep = rs.coercebrep(object)
        meshes = rc.Geometry.Mesh.CreateFromBrep(brep, g_meshParams)
        joinedMesh = rc.Geometry.Mesh()
        for mesh in meshes:
            joinedMesh.Append(mesh)
        joinedMesh.Reduce(0, False, 10, False)
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

def main():
    global g_instances
    global g_parts
    global g_indent

    #dlg = rc.UI.SaveFileDialog()
    #if not dlg.ShowSaveDialog() : return None

    #print(sys.version_info) # (2, 7, 0, 'beta', 0)

    selectedObjects = rs.SelectedObjects()

    objects = []
    origin = [0,0,0]

    for object in selectedObjects:
        name = rs.ObjectName(object)
        type = rs.ObjectType(object)
        if type == rs.filter.point:
            if name == "Origin":
                origin = rs.PointCoordinates(object)
        else:
            objects.append(object)

    print("Processing " + str(len(objects)) + " object(s)")
    print("Origin is [%.2f,%.2f,%.2f]" % (origin[0],origin[1],origin[2]))

    rootInstance = \
    {
                 "name" : "",
                 "type" : "",
        "blockInstance" : None,
              "parents" : [],
                "parts" : [],
    }

    g_instances.append(rootInstance)

    objectsCopied = rs.CopyObjects(objects, rs.VectorScale(origin, -1))
    for object in objectsCopied:
        processObject(object, [rootInstance])

    for instance in g_instances:
        parentCount = len(instance["parents"])
        if parentCount < 1:
            continue

        faceCount = 0
        for part in instance["parts"]:
            mesh = part["mesh"]
            faceCount += mesh.Faces.Count;

        indent = g_indent * (parentCount - 1)
        print("i " + indent + instance["name"]
            + ":" + instance["type"] + " " + str(faceCount))

        indent += "  "
        xform = instance["xform"]
        print("x " + indent +
            formatHexFloats([xform.M00, xform.M01, xform.M02, xform.M03]))
        print("x " + indent +
            formatHexFloats([xform.M10, xform.M11, xform.M12, xform.M13]))
        print("x " + indent +
            formatHexFloats([xform.M20, xform.M21, xform.M22, xform.M23]))

    for part in g_parts:
        mesh = part["mesh"]
        faces = mesh.Faces;
        print("p " + part["name"] + " " + str(faces.Count))

main()
