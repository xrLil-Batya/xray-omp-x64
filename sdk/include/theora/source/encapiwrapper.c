#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "apiwrapper.h"
#include "encint.h"
#include "theora/theoraenc.h"



static void th_enc_api_clear(th_api_wrapper *_api){
  if(_api->encode)th_encode_free(_api->encode);
  memset(_api,0,sizeof(*_api));
}

static void theora_encode_clear(theora_state *_te){
  if(_te->i!=NULL)theora_info_clear(_te->i);
  memset(_te,0,sizeof(*_te));
}

static int theora_encode_control(theora_state *_te,int _req,
 void *_buf,size_t _buf_sz){
  return th_encode_ctl(((th_api_wrapper *)_te->i->codec_setup)->encode,
   _req,_buf,_buf_sz);
}

static ogg_int64_t theora_encode_granule_frame(theora_state *_te,
 ogg_int64_t _gp){
  return th_granule_frame(((th_api_wrapper *)_te->i->codec_setup)->encode,_gp);
}

static double theora_encode_granule_time(theora_state *_te,ogg_int64_t _gp){
  return th_granule_time(((th_api_wrapper *)_te->i->codec_setup)->encode,_gp);
}

static const oc_state_dispatch_vtable OC_ENC_DISPATCH_VTBL={
  (oc_state_clear_func)theora_encode_clear,
  (oc_state_control_func)theora_encode_control,
  (oc_state_granule_frame_func)theora_encode_granule_frame,
  (oc_state_granule_time_func)theora_encode_granule_time,
};
