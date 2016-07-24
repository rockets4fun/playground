import sys

import Rhino as rc
import rhinoscriptsyntax as rs
import scriptcontext as sc

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

def processObject(object, parentInstances):
    global g_parts

    name = rs.ObjectName(object)
    type = rs.ObjectType(object)
    layer = rs.ObjectLayer(object)

    if type == rs.filter.instance:
        type = rs.BlockInstanceName(object)
        transform = rs.BlockInstanceXform(object)
        subObjects = rs.ExplodeBlockInstance(object)

        instance = \
        {
            "name" : name,
            "type" : type,
            "blockInstance" : object,
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

    #dlg = rc.UI.SaveFileDialog()
    #if not dlg.ShowSaveDialog() : return None

    #print sys.version_info

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
        indent = "    " * (parentCount - 1)
        print "i " + indent + instance["name"] + ":" + instance["type"]

    for part in g_parts:
        print "p " + part["name"]

main()
