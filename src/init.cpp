// Maya
#include <maya/MFnPlugin.h>

// Internal
#include "CaptureCmd.h"

MStatus initializePlugin(MObject object)
{
    MStatus status;

    MFnPlugin plugin(object, "Waaake", "1.0");

    // Register the command
    status = plugin.registerCommand(CaptureCmd::commandName(), CaptureCmd::creator, CaptureCmd::newSyntax);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}

MStatus uninitializePlugin(MObject object)
{
    MStatus status;

    MFnPlugin plugin(object);

    // Deregister
    status = plugin.deregisterCommand(CaptureCmd::commandName());
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}
