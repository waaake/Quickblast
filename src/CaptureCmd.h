#ifndef _CAPTURE_CMD_H
#define _CAPTURE_CMD_H

// STD
#include <iomanip>
#include <iostream>

// Maya
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/M3dView.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>
#include <maya/MAnimControl.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MDrawContext.h>
#include <maya/MFileObject.h>
//#include <maya/MQtUtil.h>
#include <maya/MArgList.h>


class CaptureCmd : public MPxCommand
{
 public:
    CaptureCmd();
    virtual ~CaptureCmd() override;

    MStatus doIt(const MArgList& args) override;

    // Creators
    static MSyntax newSyntax();
    static void* creator() { return new CaptureCmd(); }
    static MString commandName() { return "quickblast"; }

    // Callback
    static void captureCallback(MHWRender::MDrawContext& context, void* userData);

 protected: // Methods
    MStatus parseArgs(const MArgList& args);
    MStatus validate() const;
    MString paddedFrame(double frame) const;
    MString framePath() const;

 protected: // Members
    double m_startFrame, m_endFrame;

    MString m_basepath;     // The basepath to where the renders will be saved
    MString m_format;       // Image file format

    int m_padding;          // The frame padding

    MTime m_currentTime;    // Current frame of the renderer

    int m_width, m_height;  // Resolution

    MString m_currentPanel, m_currentRenderName;

    // Two arrays for corresponding to each others index
    MStringArray renderViews;
    MStringArray renderNames;

 private: // Notifications
    MString m_notificationName;
    MString m_notificationSemantic;
};

#endif // _CAPTURE_CMD_H
