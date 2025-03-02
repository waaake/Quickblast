// Internal
#include "CaptureCmd.h"

// STD
#include <sstream>

// Static so that these won't leak into other translation units
static const char* kStartTimeFlag = "-st";
static const char* kStartTimeFlagLong = "-startTime";

static const char* kEndTimeFlag = "-et";
static const char* kEndTimeFlagLong = "-endTime";

static const char* kBasePathFlag = "-bp";
static const char* kBasePathFlagLong = "-basepath";

static const char* kFormatFlag = "-fmt";
static const char* kFormatFlagLong = "-format";

static const char* kWidthFlag = "-w";
static const char* kWidthFlagLong = "-width";

static const char* kHeightFlag = "-h";
static const char* kHeightFlagLong = "-height";

static const char* kPaddingFlag = "-p";
static const char* kPaddingFlagLong = "-padding";

static const char* kViewFlag = "-v";
static const char* kViewFlagLong = "-view";

static const char* kRenderNameFlag = "-rn";
static const char* kRenderNameFlagLong = "-renderName";


CaptureCmd::CaptureCmd()
    : m_startFrame(1.f)
    , m_endFrame(120.f)
    , m_basepath("")
    , m_format("")
    , m_padding(4)
    , m_currentTime(0.0)
    , m_width(1280)
    , m_height(720)
    , m_currentPanel("")
    , m_currentRenderName("")
    , renderViews()
    , renderNames()
    , m_notificationName("CaptureCallback")
    , m_notificationSemantic(MHWRender::MPassContext::kEndRenderSemantic)
{   
}

CaptureCmd::~CaptureCmd()
{
}

MStatus CaptureCmd::parseArgs(const MArgList& args)
{
    MStatus status = MS::kSuccess;

    // Fetch the user data
    MArgDatabase argData(syntax(), args, &status);

    if (!status)
    {
        return MS::kFailure;
    }

    // Basepath needs to be provided all the time
    if (!argData.isFlagSet(kBasePathFlag))
    {
        return MS::kFailure;
    }

    // Fetch the basepath
    status = argData.getFlagArgument(kBasePathFlag, 0, m_basepath);

    if (argData.isFlagSet(kFormatFlag))
        status = argData.getFlagArgument(kFormatFlag, 0, m_format);
    
    if (argData.isFlagSet(kStartTimeFlag))
        status = argData.getFlagArgument(kStartTimeFlag, 0, m_startFrame);
    
    if (argData.isFlagSet(kEndTimeFlag))
        status = argData.getFlagArgument(kEndTimeFlag, 0, m_endFrame);
    
    if (argData.isFlagSet(kWidthFlag))
        status = argData.getFlagArgument(kWidthFlag, 0, m_width);
    
    if (argData.isFlagSet(kHeightFlag))
        status = argData.getFlagArgument(kHeightFlag, 0, m_height);
    
    if (argData.isFlagSet(kPaddingFlag))
        status = argData.getFlagArgument(kPaddingFlag, 0, m_padding);
    
    if (argData.isFlagSet(kViewFlag))
    {
        unsigned int usage = argData.numberOfFlagUses(kViewFlag);
        for (unsigned int i = 0; i < usage; i++)
        {
            MArgList views;
            status = argData.getFlagArgumentList(kViewFlag, i, views);

            // Ensure that we are not using something which is likely to cause a crash
            if (!status)
                return status;
            
            // Update the current view
            MString view;
            status = views.get(0, view);

            //CHECK_MSTATUS_AND_RETURN_IT(status);
            if (!status)
                return status;

            // Update the string array
            renderViews.append(view);
        }
    }

    // Get the similar number of instances of the render names
    if (argData.isFlagSet(kRenderNameFlag))
    {
        unsigned int usage = argData.numberOfFlagUses(kRenderNameFlag);
        for (unsigned int i = 0; i < usage; i++)
        {
            MArgList names;
            status = argData.getFlagArgumentList(kRenderNameFlag, i, names);

            // Ensure that we are not using something which is likely to cause a crash
            if (!status)
                return status;
            
            // Update the current renderName
            MString name;
            status = names.get(0, name);

            if (!status)
                return status;
            
            // Update the names array
            renderNames.append(name);
        }
    }

    // Should be MS::kSuccess
    return status;
}

MSyntax CaptureCmd::newSyntax()
{
    MSyntax syntax;

    /* Add flags */
    // Time flags
    syntax.addFlag(kStartTimeFlag, kStartTimeFlagLong, MSyntax::kDouble);
    syntax.addFlag(kEndTimeFlag, kEndTimeFlagLong, MSyntax::kDouble);

    // View flags
    syntax.addFlag(kViewFlag, kViewFlagLong, MSyntax::kString);
    syntax.makeFlagMultiUse(kViewFlag);

    syntax.addFlag(kRenderNameFlag, kRenderNameFlagLong, MSyntax::kString);
    syntax.makeFlagMultiUse(kRenderNameFlag);

    // File flags
    syntax.addFlag(kBasePathFlag, kBasePathFlagLong, MSyntax::kString);
    syntax.addFlag(kFormatFlag, kFormatFlagLong, MSyntax::kString);
    syntax.addFlag(kPaddingFlag, kPaddingFlagLong, MSyntax::kString);

    // Resolution flags
    syntax.addFlag(kWidthFlag, kWidthFlagLong, MSyntax::kUnsigned);
    syntax.addFlag(kHeightFlag, kHeightFlagLong, MSyntax::kUnsigned);

    // Class does not provide anything that can be queried
    syntax.enableQuery(false);

    return syntax;
}

MStatus CaptureCmd::doIt(const MArgList& args)
{
    // Init status
    MStatus status;

    // Fetch the current renderer
    // Is always going to get the static instance back
    MRenderer* renderer = MRenderer::theRenderer();

    if (!renderer)
    {
        MGlobal::displayError("Viewport 2.0 is not initialised.");
        return MS::kFailure;
    }

    M3dView view = M3dView::active3dView(&status);

    // The active view is not a 3dview to work with
    if (!status)
    {
        MGlobal::displayError("Active view is not a 3d View.");
        return MS::kFailure;
    }

    // Parse the args
    status = parseArgs(args);

    if (!status)
    {
        MGlobal::displayError("Invalid flags.");
        return MS::kFailure;
    }

    // Validate the variables before we start executing the process
    status = validate();

    if (!status)
    {
        MGlobal::displayError("Failed argument validation.");
        return MS::kFailure;
    }

    // Setup Callback
    // Send the current class instance along as a void*
    renderer->addNotification(captureCallback, m_notificationName, m_notificationSemantic, static_cast<void*>(this));

    // Temporarily disable the on screen updates
    // This basically will not update the active viewport for each frame change until the processing is completed
    renderer->setPresentOnScreen(false);

    // Set the render resolution
    renderer->setOutputTargetOverrideSize(m_width, m_height);

    // Update the frame
    // Till the time the callback is active, every frame change would trigger it as soon as the viewport rasterisation is complete
    for (m_currentTime = m_startFrame; m_currentTime <= m_endFrame; m_currentTime++)
    {
        MAnimControl::setCurrentTime(m_currentTime);

        M3dView view;

        if (renderViews.length())
        {
            for (unsigned int i = 0; i < renderViews.length(); i++)
            {
                // Current panel which is active
                m_currentPanel = renderViews[i];
                m_currentRenderName = renderNames[i];

                status = M3dView::getM3dViewFromModelPanel(m_currentPanel, view);

                if (status)
                {
                    // Update the current view
                    // This update will trigger the callback function which will capture and save the frame
                    // all=false, force=true
                    view.refresh(false, true);
                }
            }
        }
    }

    // Remove the callback
    renderer->removeNotification(m_notificationName, m_notificationSemantic);

    // Enable onscreen updates
    renderer->setPresentOnScreen(true);

    // Reset the render size override added for the playblast
    renderer->unsetOutputTargetOverrideSize();

    return MS::kSuccess;
}

MStatus CaptureCmd::validate() const
{
    // Check if the directory exists
    MFileObject directory;
    directory.setRawPath(m_basepath);

    if (!directory.exists())
    {
        MGlobal::displayError("Directory does not exist: " + m_basepath);
        return MS::kFailure;
    }

    // If the file is good to be saved with the provided name or format
    if (!m_format.length())
    {
        MGlobal::displayError("Invalid format provided.");
        return MS::kFailure;
    }

    // Can't write backwards
    if (m_endFrame < m_startFrame)
    {
        MGlobal::displayError("Invalid end frame provided");
        return MS::kFailure;
    }

    // Validate that each view has its own name to dump the capture in
    if (renderNames.length() != renderViews.length())
    {
        MGlobal::displayError("Invalid number of Render Names provided");
        return MS::kFailure;
    }

    // 1920 x 0 feels wierd
    if (!m_width || !m_height)
    {
        MGlobal::displayError("Invalid resolution provided.");
        return MS::kFailure;
    }

    // We're good to Render
    return MS::kSuccess;
}

MString CaptureCmd::paddedFrame(double frame) const
{
    std::stringstream ss;
    // Fill with zeroes based on the padding width
    ss << std::setw(m_padding) << std::setfill('0') << frame;

    // As there's no direct conversion from a stringstream to a const char*
    // Cast it to a string and then create an MString implicitly from the c_str function
    std::string framestr = ss.str();

    return framestr.c_str();
}

MString CaptureCmd::framePath() const
{
    // The padded value of the current frame
    MString pframe = paddedFrame(m_currentTime.value());

    // The fullpath to the current frame as how it will be saved out
    // e.g. /base/path/to/basedir / renderName / renderName . 1001 . png
    // framepath = basepath + / + m_fileName + / + m_fileName + . + pframe + . + m_format;
    MString framepath = m_basepath + "/" + m_currentRenderName + "/" + m_currentRenderName + "." + pframe + "." + m_format;

    return framepath;
}

void CaptureCmd::captureCallback(MHWRender::MDrawContext& context, void* userdata)
{
    // Cast the void pointer to our command class, if it can be casted to our command class instance -> we can proceed
    CaptureCmd* cmdPtr = static_cast<CaptureCmd*>(userdata);

    if (!cmdPtr)
        return;
    
    // Active renderer
    MRenderer* renderer = MRenderer::theRenderer();

    if (!renderer)
        return;
    
    // Get the current render targets from the viewport's draw context
    const MHWRender::MRenderTarget* colorTarget = context.getCurrentColorRenderTarget();

    if (colorTarget)
    {
        // Get a copy of the render target
        // Get it back as a texture for saving using the save texture method on the texture manager
        MHWRender::MTextureManager* textureManager = renderer->getTextureManager();
        MHWRender::MTexture* colorTexture = context.copyCurrentColorRenderTargetToTexture();

        // Save the color texture to file
        if (colorTexture)
        {
            textureManager->saveTexture(colorTexture, cmdPtr->framePath());
        }

        // Release Reference to the color target
        const MHWRender::MRenderTargetManager* targetManager = renderer->getRenderTargetManager();
        targetManager->releaseRenderTarget(colorTarget);
    }
}
