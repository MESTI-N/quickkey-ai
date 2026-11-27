// QuickKey AI: OpenFX frontend for a separately installed CorridorKey 2.0 engine.
// Original QuickKey glue code. No CorridorKey model or library is redistributed.
// Tested by macOS GitHub CI for compilation; requires host integration testing.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>
#include <unistd.h>
#include <spawn.h>
#include <mach-o/dyld.h>
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxProperty.h"
#include <zlib.h>
#define TINYEXR_USE_MINIZ 0
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

extern char **environ;
namespace fs = std::filesystem;
namespace {
OfxHost* host=nullptr;
OfxPropertySuiteV1* props=nullptr;
OfxImageEffectSuiteV1* effects=nullptr;
OfxParameterSuiteV1* parameters=nullptr;
constexpr char kId[]="org.quickkeyai.mac.ai";
constexpr char kSource[]="input_file";
constexpr char kEngine[]="ai_engine";
constexpr char kCache[]="cache_folder";
constexpr char kProcess[]="process_shot";
constexpr char kMode[]="display_mode";
constexpr char kStart[]="first_timeline_frame";
constexpr char kDespill[]="despill_strength";
constexpr char kRefresh[]="refresh_output";

void setHost(OfxHost* h) {host=h;}
OfxStatus load() {
 if(!host || !host->fetchSuite) return kOfxStatFailed;
 props=(OfxPropertySuiteV1*)host->fetchSuite(host->host,kOfxPropertySuite,1);
 effects=(OfxImageEffectSuiteV1*)host->fetchSuite(host->host,kOfxImageEffectSuite,1);
 parameters=(OfxParameterSuiteV1*)host->fetchSuite(host->host,kOfxParameterSuite,1);
 return props&&effects&&parameters?kOfxStatOK:kOfxStatErrMissingHostFeature;
}
void defineString(OfxParamSetHandle set,const char* key,const char* label,
                  const char* mode,const char* hint,const char* def="") {
 OfxPropertySetHandle ps=nullptr;
 parameters->paramDefine(set,kOfxParamTypeString,key,&ps);
 props->propSetString(ps,kOfxPropLabel,0,label);
 props->propSetString(ps,kOfxParamPropStringMode,0,mode);
 props->propSetString(ps,kOfxParamPropDefault,0,def);
 props->propSetString(ps,kOfxParamPropHint,0,hint);
}
void defineButton(OfxParamSetHandle set,const char* key,const char* label,const char* hint) {
 OfxPropertySetHandle ps=nullptr;
 parameters->paramDefine(set,kOfxParamTypePushButton,key,&ps);
 props->propSetString(ps,kOfxPropLabel,0,label);
 props->propSetString(ps,kOfxParamPropHint,0,hint);
}
void defineDouble(OfxParamSetHandle set,const char* key,const char* label,double def,double lo,double hi) {
 OfxPropertySetHandle ps=nullptr;
 parameters->paramDefine(set,kOfxParamTypeDouble,key,&ps);
 props->propSetString(ps,kOfxPropLabel,0,label);
 props->propSetDouble(ps,kOfxParamPropDefault,0,def);
 props->propSetDouble(ps,kOfxParamPropMin,0,lo);
 props->propSetDouble(ps,kOfxParamPropMax,0,hi);
 props->propSetDouble(ps,kOfxParamPropDisplayMin,0,lo);
 props->propSetDouble(ps,kOfxParamPropDisplayMax,0,hi);
}
OfxStatus describe(OfxImageEffectHandle e) {
 OfxPropertySetHandle p=nullptr;
 if(effects->getPropertySet(e,&p)!=kOfxStatOK) return kOfxStatFailed;
 props->propSetString(p,kOfxPropLabel,0,"QuickKey AI - CorridorKey 2.0");
 props->propSetString(p,kOfxImageEffectPluginPropGrouping,0,"QuickKey AI");
 props->propSetString(p,kOfxImageEffectPropSupportedContexts,0,kOfxImageEffectContextFilter);
 props->propSetString(p,kOfxImageEffectPropSupportedPixelDepths,0,kOfxBitDepthFloat);
 props->propSetString(p,kOfxImageEffectPropSupportedPixelDepths,1,kOfxBitDepthByte);
 props->propSetString(p,kOfxImageEffectPluginRenderThreadSafety,0,kOfxImageEffectRenderInstanceSafe);
 props->propSetInt(p,kOfxImageEffectPropSupportsTiles,0,1);
 props->propSetInt(p,kOfxImageEffectPropSupportsMultiResolution,0,0);
 return kOfxStatOK;
}
OfxStatus describeContext(OfxImageEffectHandle e) {
 OfxPropertySetHandle ps=nullptr;
 if(effects->clipDefine(e,kOfxImageEffectSimpleSourceClipName,&ps)!=kOfxStatOK) return kOfxStatFailed;
 props->propSetString(ps,kOfxImageEffectPropSupportedComponents,0,kOfxImageComponentRGBA);
 if(effects->clipDefine(e,kOfxImageEffectOutputClipName,&ps)!=kOfxStatOK) return kOfxStatFailed;
 props->propSetString(ps,kOfxImageEffectPropSupportedComponents,0,kOfxImageComponentRGBA);
 OfxParamSetHandle set=nullptr;
 if(effects->getParamSet(e,&set)!=kOfxStatOK) return kOfxStatFailed;
 defineString(set,kSource,"Source video (original)",
              kOfxParamStringIsFilePath,"Choose the original clip file, not timeline effects.");
 defineString(set,kEngine,"CorridorKey engine executable",
              kOfxParamStringIsFilePath,
              "Choose the 'corridorkey' engine executable inside the separately installed Mac desktop app.");
 defineString(set,kCache,"Output cache folder",
              kOfxParamStringIsDirectoryPath,"Choose a writable output folder unique to this shot.");
 defineDouble(set,kDespill,"Despill strength",0.5,0.0,1.0);
 defineDouble(set,kStart,"First Resolve frame (offset)",0.0,-100000.0,100000.0);
 OfxPropertySetHandle cp=nullptr;
 parameters->paramDefine(set,kOfxParamTypeChoice,kMode,&cp);
 props->propSetString(cp,kOfxPropLabel,0,"Output");
 props->propSetString(cp,kOfxParamPropChoiceOption,0,"AI Cutout (transparent)");
 props->propSetString(cp,kOfxParamPropChoiceOption,1,"AI Matte (black and white)");
 props->propSetString(cp,kOfxParamPropChoiceOption,2,"Original (no key)");
 props->propSetInt(cp,kOfxParamPropDefault,0,0);
 defineButton(set,kProcess,"Process Shot with AI",
              "Starts the independently installed CorridorKey engine and saves actual AI RGBA EXR frames.");
 defineButton(set,kRefresh,"Refresh Render",
              "Ask Resolve to reevaluate output after the external AI job finishes.");
 return kOfxStatOK;
}
std::string getString(OfxParamSetHandle set,const char* key) {
 OfxParamHandle h=nullptr;
 char* val=nullptr;
 if(parameters->paramGetHandle(set,key,&h,nullptr)!=kOfxStatOK)return "";
 if(parameters->paramGetValue(h,&val)!=kOfxStatOK || !val)return "";
 return val;
}
double getDouble(OfxParamSetHandle set,const char* key,double fallback=0) {
 OfxParamHandle h=nullptr;
 double val=fallback;
 if(parameters->paramGetHandle(set,key,&h,nullptr)==kOfxStatOK)
   parameters->paramGetValue(h,&val);
 return val;
}
int getChoice(OfxParamSetHandle set) {
 OfxParamHandle h=nullptr;int val=0;
 if(parameters->paramGetHandle(set,kMode,&h,nullptr)==kOfxStatOK)
   parameters->paramGetValue(h,&val);
 return val;
}
std::string workerPath() {
 uint32_t size=0; _NSGetExecutablePath(nullptr,&size);
 std::vector<char> buffer(size+1);
 if(_NSGetExecutablePath(buffer.data(),&size)!=0) return "";
 return (fs::path(buffer.data()).parent_path().parent_path()/"Resources"/"quickkey-worker.sh").string();
}
void launchWorker(OfxImageEffectHandle e) {
 OfxParamSetHandle set=nullptr;
 if(effects->getParamSet(e,&set)!=kOfxStatOK)return;
 auto source=getString(set,kSource),engine=getString(set,kEngine),cache=getString(set,kCache);
 if(source.empty() || engine.empty() || cache.empty())return;
 const std::string worker=workerPath();
 if(!fs::exists(worker) || !fs::exists(engine) || !fs::exists(source))return;
 std::error_code ec;fs::create_directories(cache,ec);
 if(ec)return;
 std::string running=(fs::path(cache)/".quickkey_running").string();
 if(fs::exists(running))return;
 auto despill=std::to_string(getDouble(set,kDespill,0.5));
 // posix_spawn invokes a dedicated script; AI never runs in an OFX frame-render callback.
 pid_t pid=0;
 const char* shell="/bin/bash";
 char* const argv[]={const_cast<char*>(shell),const_cast<char*>(worker.c_str()),
  const_cast<char*>(engine.c_str()),const_cast<char*>(source.c_str()),
  const_cast<char*>(cache.c_str()),const_cast<char*>(despill.c_str()),nullptr};
 posix_spawn(&pid,shell,nullptr,nullptr,argv,environ);
}
OfxStatus onChanged(OfxImageEffectHandle e,OfxPropertySetHandle inArgs) {
 char* changed=nullptr;
 if(props->propGetString(inArgs,kOfxPropName,0,&changed)!=kOfxStatOK || !changed)
   return kOfxStatReplyDefault;
 if(!std::strcmp(changed,kProcess)) launchWorker(e);
 return kOfxStatOK;
}

struct Image {
 OfxPropertySetHandle handle=nullptr;
 void* data=nullptr;int bounds[4]={0,0,0,0};int rowBytes=0;
 bool floating=false,valid=false;
};
bool getImage(OfxImageClipHandle clip,OfxTime time,Image& img) {
 if(effects->clipGetImage(clip,time,nullptr,&img.handle)!=kOfxStatOK || !img.handle)return false;
 if(props->propGetPointer(img.handle,kOfxImagePropData,0,&img.data)!=kOfxStatOK ||
    props->propGetIntN(img.handle,kOfxImagePropBounds,4,img.bounds)!=kOfxStatOK ||
    props->propGetInt(img.handle,kOfxImagePropRowBytes,0,&img.rowBytes)!=kOfxStatOK)return false;
 char *depth=nullptr,*components=nullptr;
 if(props->propGetString(img.handle,kOfxImageEffectPropPixelDepth,0,&depth)!=kOfxStatOK ||
    props->propGetString(img.handle,kOfxImageEffectPropComponents,0,&components)!=kOfxStatOK)return false;
 if(!depth || !components || std::strcmp(components,kOfxImageComponentRGBA))return false;
 if(!std::strcmp(depth,kOfxBitDepthFloat))img.floating=true;
 else if(!std::strcmp(depth,kOfxBitDepthByte))img.floating=false;
 else return false;
 img.valid=img.data!=nullptr;
 return img.valid;
}
void* pixel(const Image& img,int x,int y) {
 if(!img.valid || x<img.bounds[0] || x>=img.bounds[2] || y<img.bounds[1] || y>=img.bounds[3])return nullptr;
 return (char*)img.data+(y-img.bounds[1])*img.rowBytes+
        (x-img.bounds[0])*(img.floating?16:4);
}
std::vector<fs::path> allExrs(const std::string& cache) {
 std::vector<fs::path> found;
 std::error_code ec;
 if(!fs::exists(fs::path(cache)/".quickkey_ready",ec))return found;
 for(fs::recursive_directory_iterator it(cache,fs::directory_options::skip_permission_denied,ec),end;
     !ec && it!=end;it.increment(ec)) {
   if(it->is_regular_file(ec) && it->path().extension()==".exr" &&
      it->path().filename().string().find("_cutout.")!=std::string::npos)
       found.push_back(it->path());
 }
 std::sort(found.begin(),found.end());
 return found;
}
float clamp01(float x) {return std::max(0.f,std::min(1.f,x));}
OfxStatus render(OfxImageEffectHandle e,OfxPropertySetHandle args) {
 OfxTime time=0;int win[4]={0,0,0,0};
 if(props->propGetDouble(args,kOfxPropTime,0,&time)!=kOfxStatOK ||
    props->propGetIntN(args,kOfxImageEffectPropRenderWindow,4,win)!=kOfxStatOK)
    return kOfxStatFailed;
 OfxImageClipHandle srcClip=nullptr,dstClip=nullptr;
 if(effects->clipGetHandle(e,kOfxImageEffectSimpleSourceClipName,&srcClip,nullptr)!=kOfxStatOK ||
    effects->clipGetHandle(e,kOfxImageEffectOutputClipName,&dstClip,nullptr)!=kOfxStatOK)
    return kOfxStatFailed;
 Image src,dst;bool srcOk=getImage(srcClip,time,src),dstOk=getImage(dstClip,time,dst);
 if(!dstOk || (srcOk && dst.floating!=src.floating)) {
  if(src.handle)effects->clipReleaseImage(src.handle);
  if(dst.handle)effects->clipReleaseImage(dst.handle);
  return kOfxStatErrImageFormat;
 }
 OfxParamSetHandle set=nullptr;effects->getParamSet(e,&set);
 int mode=set?getChoice(set):2;
 std::vector<float> rgba;int w=0,h=0;
 if(set && mode!=2) {
   auto cache=getString(set,kCache);
   auto files=allExrs(cache);
   long long index=std::llround(time-getDouble(set,kStart));
   if(index>=0 && (size_t)index<files.size()) {
     float* ptr=nullptr;const char* err=nullptr;
     int result=LoadEXR(&ptr,&w,&h,files[(size_t)index].string().c_str(),&err);
     if(result==TINYEXR_SUCCESS && ptr) {
       rgba.assign(ptr,ptr+(size_t)w*(size_t)h*4);
       free(ptr);
     }
     if(err)FreeEXRErrorMessage(err);
   }
 }
 bool hasAI=!rgba.empty() && (dst.bounds[2]-dst.bounds[0])==w &&
                             (dst.bounds[3]-dst.bounds[1])==h;
 for(int y=win[1];y<win[3];++y) {
  if(effects->abort(e))break;
  for(int x=win[0];x<win[2];++x) {
    void* out=pixel(dst,x,y);
    if(!out)continue;
    void* in=pixel(src,x,y);
    float c[4]={0,0,0,0};
    if(in) {
      if(dst.floating){for(int i=0;i<4;++i)c[i]=((float*)in)[i];}
      else {for(int i=0;i<4;++i)c[i]=((unsigned char*)in)[i]/255.f;}
    }
    if(hasAI) {
      // OpenEXR scanline origin is top-left; OFX image coordinates are bottom-left.
      size_t ix=(size_t)(x-dst.bounds[0]),iy=(size_t)(dst.bounds[3]-1-y);
      size_t off=(iy*(size_t)w+ix)*4;
      if(ix<(size_t)w && iy<(size_t)h) {
       for(int j=0;j<4;++j)c[j]=rgba[off+j];
       if(mode==1){float a=c[3];c[0]=c[1]=c[2]=a;c[3]=1.f;}
      }
    }
    if(dst.floating){for(int i=0;i<4;++i)((float*)out)[i]=c[i];}
    else {for(int i=0;i<4;++i)((unsigned char*)out)[i]=(unsigned char)std::lround(clamp01(c[i])*255.f);}
  }
 }
 if(src.handle)effects->clipReleaseImage(src.handle);
 if(dst.handle)effects->clipReleaseImage(dst.handle);
 return kOfxStatOK;
}
OfxStatus entry(const char* action,const void* h,OfxPropertySetHandle in,OfxPropertySetHandle) {
 if(!action)return kOfxStatErrBadHandle;
 if(!std::strcmp(action,kOfxActionLoad))return load();
 if(!std::strcmp(action,kOfxActionUnload))return kOfxStatOK;
 if(!effects || !props || !parameters)return kOfxStatErrMissingHostFeature;
 if(!std::strcmp(action,kOfxActionDescribe))return describe((OfxImageEffectHandle)h);
 if(!std::strcmp(action,kOfxImageEffectActionDescribeInContext))return describeContext((OfxImageEffectHandle)h);
 if(!std::strcmp(action,kOfxImageEffectActionInstanceChanged))return onChanged((OfxImageEffectHandle)h,in);
 if(!std::strcmp(action,kOfxImageEffectActionRender))return render((OfxImageEffectHandle)h,in);
 return kOfxStatReplyDefault;
}
OfxPlugin plugin={kOfxImageEffectPluginApi,kOfxImageEffectPluginApiVersion,kId,0,2,setHost,entry};
}
extern "C" {
__attribute__((visibility("default"))) int OfxGetNumberOfPlugins(){return 1;}
__attribute__((visibility("default"))) OfxPlugin* OfxGetPlugin(int n){return n==0?&plugin:nullptr;}
}
