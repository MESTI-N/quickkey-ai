// QuickKey AI Foundation: native OpenFX RGBA diagnostic (no AI inference).
// Tests transparent output without modifying clips on other tracks.
#include <cstring>
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxProperty.h"

namespace {
OfxHost* host = nullptr;
OfxPropertySuiteV1* props = nullptr;
OfxImageEffectSuiteV1* effects = nullptr;
OfxParameterSuiteV1* parameters = nullptr;
const char* const pluginId = "org.quickkeyai.mac.foundation";
const char* const modeParam = "quickkey_test_mode";

void setHost(OfxHost* h) { host = h; }
OfxStatus load() {
  if (!host || !host->fetchSuite) return kOfxStatFailed;
  props = (OfxPropertySuiteV1*)host->fetchSuite(host->host,kOfxPropertySuite,1);
  effects = (OfxImageEffectSuiteV1*)host->fetchSuite(host->host,kOfxImageEffectSuite,1);
  parameters = (OfxParameterSuiteV1*)host->fetchSuite(host->host,kOfxParameterSuite,1);
  return props && effects && parameters ? kOfxStatOK : kOfxStatErrMissingHostFeature;
}
OfxStatus describe(OfxImageEffectHandle e) {
  OfxPropertySetHandle ps=nullptr;
  if (effects->getPropertySet(e,&ps)!=kOfxStatOK) return kOfxStatFailed;
  props->propSetString(ps,kOfxPropLabel,0,"QuickKey AI - Foundation Test");
  props->propSetString(ps,kOfxImageEffectPluginPropGrouping,0,"QuickKey AI");
  props->propSetString(ps,kOfxImageEffectPropSupportedContexts,0,kOfxImageEffectContextFilter);
  props->propSetString(ps,kOfxImageEffectPropSupportedPixelDepths,0,kOfxBitDepthFloat);
  props->propSetString(ps,kOfxImageEffectPropSupportedPixelDepths,1,kOfxBitDepthByte);
  props->propSetString(ps,kOfxImageEffectPluginRenderThreadSafety,0,kOfxImageEffectRenderInstanceSafe);
  props->propSetInt(ps,kOfxImageEffectPropSupportsTiles,0,1);
  props->propSetInt(ps,kOfxImageEffectPropSupportsMultiResolution,0,1);
  return kOfxStatOK;
}
OfxStatus describeContext(OfxImageEffectHandle e) {
  OfxPropertySetHandle cs=nullptr;
  if (effects->clipDefine(e,kOfxImageEffectSimpleSourceClipName,&cs)!=kOfxStatOK)
    return kOfxStatFailed;
  props->propSetString(cs,kOfxImageEffectPropSupportedComponents,0,kOfxImageComponentRGBA);
  if (effects->clipDefine(e,kOfxImageEffectOutputClipName,&cs)!=kOfxStatOK)
    return kOfxStatFailed;
  props->propSetString(cs,kOfxImageEffectPropSupportedComponents,0,kOfxImageComponentRGBA);
  OfxParamSetHandle ps=nullptr;
  if (effects->getParamSet(e,&ps)!=kOfxStatOK) return kOfxStatFailed;
  OfxPropertySetHandle pp=nullptr;
  if (parameters->paramDefine(ps,kOfxParamTypeChoice,modeParam,&pp)!=kOfxStatOK)
    return kOfxStatFailed;
  props->propSetString(pp,kOfxPropLabel,0,"Test Output");
  props->propSetString(pp,kOfxParamPropChoiceOption,0,"Original (pass-through)");
  props->propSetString(pp,kOfxParamPropChoiceOption,1,"Transparent (reveal below)");
  props->propSetString(pp,kOfxParamPropChoiceOption,2,"Show incoming alpha (grayscale)");
  props->propSetInt(pp,kOfxParamPropDefault,0,0);
  props->propSetString(pp,kOfxParamPropHint,0,"Diagnostic only. No AI processing.");
  return kOfxStatOK;
}

struct Image {
  OfxPropertySetHandle handle=nullptr;
  void* data=nullptr;
  int bounds[4]={0,0,0,0};
  int rowBytes=0;
  bool floating=false;
  bool valid=false;
};
bool getImage(OfxImageClipHandle clip,OfxTime time,Image& img) {
  if (effects->clipGetImage(clip,time,nullptr,&img.handle)!=kOfxStatOK || !img.handle)
    return false;
  if (props->propGetPointer(img.handle,kOfxImagePropData,0,&img.data)!=kOfxStatOK ||
      props->propGetIntN(img.handle,kOfxImagePropBounds,4,img.bounds)!=kOfxStatOK ||
      props->propGetInt(img.handle,kOfxImagePropRowBytes,0,&img.rowBytes)!=kOfxStatOK)
    return false;
  char* depth=nullptr;
  char* components=nullptr;
  if (props->propGetString(img.handle,kOfxImageEffectPropPixelDepth,0,&depth)!=kOfxStatOK ||
      props->propGetString(img.handle,kOfxImageEffectPropComponents,0,&components)!=kOfxStatOK)
    return false;
  if (!depth || !components || std::strcmp(components,kOfxImageComponentRGBA)) return false;
  if (!std::strcmp(depth,kOfxBitDepthFloat)) img.floating=true;
  else if (!std::strcmp(depth,kOfxBitDepthByte)) img.floating=false;
  else return false;
  img.valid=img.data!=nullptr;
  return img.valid;
}
void* pixel(const Image& img,int x,int y) {
  if (!img.valid || x<img.bounds[0] || x>=img.bounds[2] ||
      y<img.bounds[1] || y>=img.bounds[3]) return nullptr;
  int bpp=img.floating ? 16 : 4;
  return (char*)img.data+(y-img.bounds[1])*img.rowBytes+(x-img.bounds[0])*bpp;
}
template<class T>
void writePixel(const T* src,T* dst,int mode,T maxValue) {
  if (!dst) return;
  if (!src) { for (int i=0;i<4;++i) dst[i]=0; return; }
  if (mode==1) {
    for (int i=0;i<3;++i) dst[i]=src[i];
    dst[3] = 0;
  } else if (mode==2) {
    for (int i=0;i<3;++i) dst[i]=src[3];
    dst[3]=maxValue;
  } else {
    for (int i=0;i<4;++i) dst[i]=src[i];
  }
}
OfxStatus render(OfxImageEffectHandle e,OfxPropertySetHandle args) {
  OfxTime time=0;
  int win[4]={0,0,0,0};
  if (props->propGetDouble(args,kOfxPropTime,0,&time)!=kOfxStatOK ||
      props->propGetIntN(args,kOfxImageEffectPropRenderWindow,4,win)!=kOfxStatOK)
    return kOfxStatFailed;
  OfxImageClipHandle srcClip=nullptr,dstClip=nullptr;
  if (effects->clipGetHandle(e,kOfxImageEffectSimpleSourceClipName,&srcClip,nullptr)!=kOfxStatOK ||
      effects->clipGetHandle(e,kOfxImageEffectOutputClipName,&dstClip,nullptr)!=kOfxStatOK)
    return kOfxStatFailed;
  OfxParamSetHandle paramSet=nullptr;
  OfxParamHandle modeHandle=nullptr;
  int mode=0;
  if (effects->getParamSet(e,&paramSet)==kOfxStatOK &&
      parameters->paramGetHandle(paramSet,modeParam,&modeHandle,nullptr)==kOfxStatOK)
    parameters->paramGetValueAtTime(modeHandle,time,&mode);
  Image src,dst;
  bool haveSrc=getImage(srcClip,time,src);
  bool haveDst=getImage(dstClip,time,dst);
  if (!haveDst || (haveSrc && src.floating!=dst.floating)) {
    if (src.handle) effects->clipReleaseImage(src.handle);
    if (dst.handle) effects->clipReleaseImage(dst.handle);
    return kOfxStatErrImageFormat;
  }
  for (int y=win[1];y<win[3];++y) {
    if (effects->abort(e)) break;
    for (int x=win[0];x<win[2];++x) {
      void* out=pixel(dst,x,y);
      void* in=pixel(src,x,y);
      if (dst.floating)
        writePixel((const float*)in,(float*)out,mode,1.0f);
      else
        writePixel((const unsigned char*)in,(unsigned char*)out,mode,(unsigned char)255);
    }
  }
  if (src.handle) effects->clipReleaseImage(src.handle);
  if (dst.handle) effects->clipReleaseImage(dst.handle);
  return kOfxStatOK;
}
OfxStatus mainEntry(const char* action,const void* effect,
                    OfxPropertySetHandle inArgs,OfxPropertySetHandle) {
  if (!action) return kOfxStatErrBadHandle;
  if (!std::strcmp(action,kOfxActionLoad)) return load();
  if (!std::strcmp(action,kOfxActionUnload)) return kOfxStatOK;
  if (!effects || !props || !parameters) return kOfxStatErrMissingHostFeature;
  if (!std::strcmp(action,kOfxActionDescribe)) return describe((OfxImageEffectHandle)effect);
  if (!std::strcmp(action,kOfxImageEffectActionDescribeInContext))
    return describeContext((OfxImageEffectHandle)effect);
  if (!std::strcmp(action,kOfxImageEffectActionRender))
    return render((OfxImageEffectHandle)effect,inArgs);
  return kOfxStatReplyDefault;
}
OfxPlugin plugin={kOfxImageEffectPluginApi,kOfxImageEffectPluginApiVersion,
                  pluginId,0,1,setHost,mainEntry};
}
extern "C" {
__attribute__((visibility("default"))) int OfxGetNumberOfPlugins() {return 1;}
__attribute__((visibility("default"))) OfxPlugin* OfxGetPlugin(int n) {
  return n==0 ? &plugin : nullptr;
}
}
