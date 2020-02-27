#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h>

#include "TSDRPlugin.h"
#include "TSDRCodes.h"

#include <stdint.h>

#include "osdetect.h"
#include "timer.h"

#define str_eq(s1,s2)  (!strcmp ((s1),(s2)))

#define MAX_ERRORS (5)

#define TYPE_FLOAT (0)
#define TYPE_BYTE (1)
#define TYPE_SHORT (2)
#define TYPE_UBYTE (3)
#define TYPE_USHORT (4)

#define PERFORMANCE_BENCHMARK (0)
#define ENABLE_LOOP (1)

#define TIME_STRETCH (1)
#define SAMPLES_TO_READ_AT_ONCE (512*1024)
#define MAX_SAMP_RATE (1000e6)

TickTockTimer_t timer;
volatile int working = 0;

int type = -1;
int sizepersample = -1;

uint32_t samplerate = 0;
char filename[300];

#if !WINHEAD
#include <unistd.h>
#endif

void thread_sleep(uint32_t milliseconds) {
#if WINHEAD
	Sleep(milliseconds);
#else
	usleep(1000 * milliseconds);
#endif
}

int errormsg_code;
char * errormsg;
int errormsg_size = 0;
#define RETURN_EXCEPTION(message, status) {announceexception(message, status); return status;}
#define RETURN_OK() {errormsg_code = TSDR_OK; return TSDR_OK;}

static inline void announceexception(const char * message, int status) {
	errormsg_code = status;
	if (status == TSDR_OK) return;

	const int length = strlen(message);
	if (errormsg_size == 0) {
			errormsg_size = length;
			errormsg = (char *) malloc(length+1);
		} else if (length > errormsg_size) {
			errormsg_size = length;
			errormsg = (char *) realloc((void*) errormsg, length+1);
		}
	strcpy(errormsg, message);
}

char TSDRPLUGIN_API __stdcall * tsdrplugin_getlasterrortext(void) {
	if (errormsg_code == TSDR_OK)
		return NULL;
	else
		return errormsg;
}

void TSDRPLUGIN_API __stdcall tsdrplugin_getName(char * name) {
	strcpy(name, "TSDR Raw File Source Plugin");
}

uint32_t TSDRPLUGIN_API __stdcall tsdrplugin_setsamplerate(uint32_t rate) {
  printf("TCP Source: Requested sample rate %ul\n",rate);
	return samplerate;
}

uint32_t TSDRPLUGIN_API __stdcall tsdrplugin_getsamplerate() {
  printf("TCP Source: Fetching sample rate %ul\n",samplerate);
	return samplerate;
}

int TSDRPLUGIN_API __stdcall tsdrplugin_setbasefreq(uint32_t freq) {
	RETURN_OK();
	return 0; // to avoid getting warning from stupid Eclpse
}

int TSDRPLUGIN_API __stdcall tsdrplugin_stop(void) {
	working = 0;
	RETURN_OK();
	return 0; // to avoid getting warning from stupid Eclpse
}

int TSDRPLUGIN_API __stdcall tsdrplugin_setgain(float gain) {
	RETURN_OK();
	return 0; // to avoid getting warning from stupid Eclpse
}

int TSDRPLUGIN_API __stdcall tsdrplugin_init(const char * params) {
  type = TYPE_FLOAT;
  sizepersample = 6;
 
  printf("TCP Source: tsdrplugin_init called with parameters '%s'\n",params);
 
	samplerate = (uint32_t) 8000000;

	RETURN_OK();
	return 0; // to avoid getting warning from stupid Eclpse
}

#define PORT 1094

int TSDRPLUGIN_API __stdcall tsdrplugin_readasync(tsdrplugin_readasync_function cb, void *ctx) {
	working = 1;
	int i;

	if (sizepersample == -1)
		RETURN_EXCEPTION("Plugin was not initialized properly.", TSDR_PLUGIN_PARAMETERS_WRONG);

  printf("TCP Source: connecting to localhost:%d...\n",PORT);

  int sock = 0, valread; 
  struct sockaddr_in serv_addr; 
  // char buffer[1024] = {0}; 
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
  { 
    printf("TCP Source: readasync: socket call failed\n"); 
		RETURN_EXCEPTION("Plugin was not initialized properly.", TSDR_PLUGIN_PARAMETERS_WRONG);
    return 0; 
  } 

  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_port = htons(PORT); 
  // Convert IPv4 and IPv6 addresses from text to binary form 
  if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
  { 
    printf("TCP Source: readasync: inet_pton call failed\n"); 
		RETURN_EXCEPTION("Plugin was not initialized properly.", TSDR_PLUGIN_PARAMETERS_WRONG);
    return 0; 
  }

  while (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
  {
    printf("TCP Source: readasync: connect failed, back off and retry...\n"); 
    sleep(3);
  } 
  
  printf("TCP Source: ready to read!\n");

	const size_t bytestoread = SAMPLES_TO_READ_AT_ONCE * sizepersample;
	char * buf = (char *) malloc(bytestoread);
	float * outbuf = (float *) malloc(sizeof(float) * SAMPLES_TO_READ_AT_ONCE);

	while (working) {
		size_t to_read = bytestoread;
		size_t total_read = 0;
		int errors = 0;

    while (to_read) {
      size_t readx = read(sock,&buf[total_read],to_read);
      to_read -= readx;
      total_read += readx;
    }

    // printf("%d\n",read(sock,buf,bytestoread));
    if (working)
    {
		  memcpy(outbuf, buf, bytestoread);
		  cb(outbuf, SAMPLES_TO_READ_AT_ONCE, ctx, 0);
    }

    /*
		if (working) {

			switch (type) {
			case TYPE_FLOAT:
				memcpy(outbuf, buf, bytestoread);
				break;
			case TYPE_BYTE:
				for (i = 0; i < SAMPLES_TO_READ_AT_ONCE; i++)
					outbuf[i] = ((int8_t) buf[i]) / 128.0;
				break;
			case TYPE_SHORT:
				for (i = 0; i < SAMPLES_TO_READ_AT_ONCE; i++)
					outbuf[i] = (*((int16_t *) (&buf[i*sizepersample]))) / 32767.0;
				break;
			case TYPE_UBYTE:
				for (i = 0; i < SAMPLES_TO_READ_AT_ONCE; i++)
					outbuf[i] = (((uint8_t) buf[i]) - 128) / 128.0;
				break;
			case TYPE_USHORT:
				for (i = 0; i < SAMPLES_TO_READ_AT_ONCE; i++)
					outbuf[i] = ((*((uint16_t *) (&buf[i*sizepersample])))-32767) / 32767.0;
				break;
			}


		}
  */
	}

	free(buf);
	free(outbuf);
  close(sock);
	// fclose(file);

	RETURN_OK();
	return 0; // to avoid getting warning from stupid Eclpse
}

void TSDRPLUGIN_API __stdcall tsdrplugin_cleanup(void) {

}
