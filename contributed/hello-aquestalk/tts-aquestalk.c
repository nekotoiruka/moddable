#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values
#include "aquestalk.h"

#define LEN_FRAME 128
uint32_t *workbuf = 0;
char *licenseKey = "XXX-XXX-XXX";

typedef struct {
	uint32_t *workbuf;
} TtsRecord, *Aques;

void xs_aques(xsMachine *the)
{
	Aques aques;
	workbuf = (uint32_t *)malloc(AQ_SIZE_WORKBUF * sizeof(uint32_t));
	if (workbuf == 0) {
		xsUnknownError("No heap memory");
	}
	aques->workbuf = workbuf;

	int iret = CAqTkPicoF_Init(aques->workbuf, LEN_FRAME, licenseKey);
	if(iret){
		xsUnknownError("AquesTalk init error");
	}
  return;
}

void xs_aques_destructor(void *data)
{
	if (data){
		Aques aques = (Aques) data;
		free(aques->workbuf);
	}
}

void xs_aques_set_speech(xsMachine *the)
{
	uint8_t *str = (xsmcArgc > 0 && xsmcTest(xsArg(0))) ? (uint8_t *)xsmcToString(xsArg(0)) : (uint8_t *)"yukku'ri/_shiteitte'ne.";
	int speed = (xsmcArgc > 1 && xsmcTest(xsArg(1))) ? xsmcToInteger(xsArg(1)) : 80;
	int iret = CAqTkPicoF_SetKoe(str, speed, 256);
	if (iret) {
		xsUnknownError("fail to set speech");
	}
  return;
}

void xs_aques_synthe_frame(xsMachine *the) 
{
	uint16_t *data = (uint16_t *)xsmcGetHostData(xsArg(0));
	uint16_t offset = xsmcArgc > 1 ? xsmcToInteger(xsArg(1)) : 0;
	data += offset;

	int16_t len;
	int iret;
	iret = CAqTkPicoF_SyntheFrame(data, &len);
	if (iret) {
		xsmcSetInteger(xsResult, -1);
	} else {
		xsmcSetInteger(xsResult, len);
	}
}