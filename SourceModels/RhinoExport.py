import sys

import Rhino as rc
import rhinoscriptsyntax as rs
import scriptcontext as sc

typeStr = \
{
    0: "unknown",
    1: "point",
    2: "point cloud",
    4: "curve",
    8: "surface",
    16: "polysurface",
    32: "mesh",
    256: "light",
    512: "annotation",
    4096: "instance",
    8192: "text dot object",
    16384: "grip object",
    32768: "detail",
    65536: "hatch",
    131072: "morph control",
    134217728: "cage",
    268435456: "phantom",
    536870912: "clipping plane",
}

meshParams = rc.Geometry.MeshingParameters.Smooth

def processObject(object, parents, types):
    name = rs.ObjectName(object)
    type = rs.ObjectType(object)
    layer = rs.ObjectLayer(object)

    if type == rs.filter.instance:
        type = rs.BlockInstanceName(object)
        transform = rs.BlockInstanceXform(object)
        subObjects = rs.ExplodeBlockInstance(object)
        for subObject in subObjects:
            processObject(subObject, parents + [name], types + [type])
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
        meshes = rc.Geometry.Mesh.CreateFromBrep(brep, meshParams)
        joinedMesh = rc.Geometry.Mesh()
        for mesh in meshes:
            joinedMesh.Append(mesh)
        joinedMesh.Reduce(0, False, 10, False)
        joinedMeshGuid = sc.doc.Objects.AddMesh(joinedMesh)
        fullName  = '.'.join(parents + [name])
        if types:
            fullName += ":" + types[-1];
        rs.ObjectName(joinedMeshGuid, fullName)
        rs.ObjectMaterialSource(joinedMeshGuid, 1)
        rs.ObjectMaterialIndex(joinedMeshGuid, rs.LayerMaterialIndex(layer))

    rs.DeleteObject(object)

def main():
    #dlg = rc.UI.SaveFileDialog()
    #if not dlg.ShowSaveDialog() : return None

    selectedObjects = rs.SelectedObjects()

    objects = []
    origin = [0,0,0]

    for object in selectedObjects:
        name = rs.ObjectName(object)
        type = rs.ObjectType(object);
        if type == rs.filter.point:
            if name == "Origin":
                origin = rs.PointCoordinates(object)
        else:
            objects.append(object)

    print("Processing " + str(len(objects)) + " object(s)")
    print("Origin is [%.2f,%.2f,%.2f]" % (origin[0],origin[1],origin[2]))

    objectsCopied = rs.CopyObjects(objects, rs.VectorScale(origin, -1))
    for object in objectsCopied:
        processObject(object, [], [])

main()
