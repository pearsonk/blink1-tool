/*
 * blink(1) C library -- 
 *
 *
 * 2012, Tod E. Kurt, http://todbot.com/blog/ , http://thingm.com/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blink1-lib.h"

#define blink1_report_id 1

#define pathmax 16
#define pathstrmax 128
#define serialmax (8 + 1) 

static char blink1_cached_paths[pathmax][pathstrmax]; 
static int blink1_cached_count = 0;
static wchar_t blink1_cached_serials[pathmax][serialmax];

static int blink1_enable_degamma = 1;

//----------------------------------------------------------------------------

//
int blink1_enumerate(void)
{
    return blink1_enumerateByVidPid( blink1_vid(), blink1_pid() );
}

// get all matching devices by VID/PID pair
int blink1_enumerateByVidPid(int vid, int pid)
{
    struct hid_device_info *devs, *cur_dev;

    int p = 0;
	devs = hid_enumerate(vid, pid);
	cur_dev = devs;	
	while (cur_dev) {
        if( (cur_dev->vendor_id != 0 && cur_dev->product_id != 0) &&  
            (cur_dev->vendor_id == vid && cur_dev->product_id == pid) ) { 
            strcpy( blink1_cached_paths[p], cur_dev->path );
            wcscpy( blink1_cached_serials[p], cur_dev->serial_number );
            //blink1_cached_serialnumbs[p] = {0};
            //wcstombs( blink1_cached_serialnums[p], cur_dev->serial );
            p++;
        }
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);
    
    blink1_cached_count = p;
    
    blink1_sortDevs();

    return p;
}

//
int blink1_getCachedCount(void)
{
    return blink1_cached_count;
}

//
const char* blink1_getCachedPath(int i)
{
    return blink1_cached_paths[i];    
}
//
const wchar_t* blink1_getCachedSerial(int i)
{
    return blink1_cached_serials[i];    
}

//
hid_device* blink1_openByPath(const char* path)
{
    if( path == NULL || strlen(path) == 0 ) return NULL;
	hid_device* handle = hid_open_path( path ); 
    return handle;
}

//
hid_device* blink1_openBySerial(const wchar_t* serial)
{
    if( serial == NULL || wcslen(serial) == 0 ) return NULL;
    int vid = blink1_vid();
    int pid = blink1_pid();
    
	hid_device* handle = hid_open(vid,pid, serial ); 
    return handle;
}

//
hid_device* blink1_openById( int i ) 
{ 
    return blink1_openByPath( blink1_getCachedPath(i) );
}

//
hid_device* blink1_open(void)
{
    int vid = blink1_vid();
    int pid = blink1_pid();

	hid_device* handle = hid_open(vid,pid, NULL);  // FIXME?

    return handle;
}

//
// FIXME: search through blink1s list to zot it too?
void blink1_close( hid_device* dev )
{
    if( dev != NULL ) 
        hid_close(dev);
    dev = NULL;
}

//
int blink1_write( hid_device* dev, void* buf, int len)
{
    //for( int i=0; i<len; i++) printf("0x%2.2x,", ((uint8_t*)buf)[i]);
    //printf("\n");

    if( dev==NULL ) {
        return -1; // BLINK1_ERR_NOTOPEN;
    }
    int rc = hid_send_feature_report( dev, buf, len );
    return rc;
}

// len should contain length of buf
// after call, len will contain actual len of buf read
int blink1_read( hid_device* dev, void* buf, int len)
{
    if( dev==NULL ) {
        return -1; // BLINK1_ERR_NOTOPEN;
    }
    int rc = hid_send_feature_report(dev, buf, len); // FIXME: check rc

    if( (rc = hid_get_feature_report(dev, buf, len) == -1) ) {
        fprintf(stderr,"error reading data: %s\n",blink1_error_msg(rc));
    }
    return rc;
}


// -------------------------------------------------------------------------
// everything below here doesn't need to know about USB details
// except for a "hid_device*"
// -------------------------------------------------------------------------

#include <unistd.h>

//
int blink1_getSerialNumber(hid_device *dev, char* buf)
{
    if( dev == NULL ) return -1;
    /*
    wchar_t* wbuf = dev->serial_number;
    int i=0;
    while( wbuf ) { 
        buf[i++] = *wbuf;
    }
    return i;
    */
    return -1;
}

//
int blink1_getVersion(hid_device *dev)
{
    char buf[9] = {blink1_report_id, 'v' };
    int len = sizeof(buf);

	//hid_set_nonblocking(dev, 0);
    int rc = blink1_write(dev, buf, sizeof(buf));
    usleep(50*1000); // FIXME
    if( rc != -1 ) // no error
        rc = blink1_read(dev, buf, len);
    if( rc != -1 ) // also no error
        rc = ((buf[3]-'0') * 100) + (buf[4]-'0'); 
    // rc is now version number or error  
    // FIXME: we don't know vals of errcodes
    return rc;
}

//
int blink1_eeread(hid_device *dev, uint16_t addr, uint8_t* val)
{
    char buf[9] = {blink1_report_id, 'e', addr };
    int len = sizeof(buf);

    int rc = blink1_write(dev, buf, len );
    usleep(50*1000); // FIXME
    if( rc != -1 ) // no error
        rc = blink1_read(dev, buf, len );
    if( rc != -1 ) 
        *val = buf[3];
    return rc;
}

//
int blink1_eewrite(hid_device *dev, uint16_t addr, uint8_t val)
{
    char buf[9] = {blink1_report_id, 'E', addr, val };

    int rc = blink1_write(dev, buf, sizeof(buf) );
        
    return rc;
}

//
int blink1_fadeToRGB(hid_device *dev,  uint16_t fadeMillis,
                     uint8_t r, uint8_t g, uint8_t b)
{
    int dms = fadeMillis/10;  // millis_divided_by_10

    char buf[9];

    buf[0] = blink1_report_id;     // report id
    buf[1] = 'c';   // command code for 'fade to rgb'
    buf[2] = ((blink1_enable_degamma) ? blink1_degamma(r) : r );
    buf[3] = ((blink1_enable_degamma) ? blink1_degamma(g) : g );
    buf[4] = ((blink1_enable_degamma) ? blink1_degamma(b) : b );
    buf[5] = (dms >> 8);
    buf[6] = dms % 0xff;

    int rc = blink1_write(dev, buf, sizeof(buf) );

    return rc; 
}

//
int blink1_setRGB(hid_device *dev, uint8_t r, uint8_t g, uint8_t b )
{
    char buf[9];

    buf[0] = blink1_report_id;     // report id
    buf[1] = 'n';   // command code for "set rgb now"
    buf[2] = ((blink1_enable_degamma) ? blink1_degamma(r) : r );     // red
    buf[3] = ((blink1_enable_degamma) ? blink1_degamma(g) : g );     // grn
    buf[4] = ((blink1_enable_degamma) ? blink1_degamma(b) : b );     // blu
    
    int rc = blink1_write(dev, buf, sizeof(buf) );
    /*
    if( rc == -1 ) 
        fprintf(stderr,"error writing data: %s\n",blink1_error_msg(rc));
    */
    return rc;  // FIXME: remove fprintf
}

//
int blink1_nightlight(hid_device *dev, uint8_t on)
{
    char buf[9] = { blink1_report_id, 'N', on };

    int rc = blink1_write(dev, buf, sizeof(buf) );
    
    return rc;
}

//
int blink1_serverdown(hid_device *dev, uint8_t on, uint16_t millis)
{
    int dms = millis/10;  // millis_divided_by_10

    char buf[9] = {blink1_report_id, 'D', on, (dms>>8), (dms % 0xff) };

    int rc = blink1_write(dev, buf, sizeof(buf) );
    return rc;
}

//
int blink1_writePatternLine(hid_device *dev, uint16_t fadeMillis, 
                            uint8_t r, uint8_t g, uint8_t b, 
                            uint8_t pos)
{
    int dms = fadeMillis/10;  // millis_divided_by_10
    char buf[9] = {blink1_report_id, 'P', r,g,b, (dms>>8), (dms % 0xff), pos };
    int rc = blink1_write(dev, buf, sizeof(buf) );
    return rc;
}


//
int readUUID( hid_device* dev, uint8_t** uuid )
{
    return -1;
}

int setUUID( hid_device* dev, uint8_t* uuid )
{
    return -1;
}


/* ------------------------------------------------------------------------- */

// FIXME: this is wrong
uint8_t degamma_lookup[256] = { 
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,2,2,2,2,2,3,3,3,3,4,
  4,4,4,5,5,5,5,6,6,6,7,7,7,8,8,9,
  9,9,10,10,11,11,11,12,12,13,13,14,14,15,15,16,
  16,17,17,18,18,19,19,20,20,21,22,22,23,23,24,25,
  25,26,27,27,28,29,29,30,31,31,32,33,33,34,35,36,
  36,37,38,39,40,40,41,42,43,44,44,45,46,47,48,49,
  50,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
  65,66,67,68,69,70,71,72,73,74,75,76,77,79,80,81,
  82,83,84,85,87,88,89,90,91,93,94,95,96,97,99,100,
  101,102,104,105,106,108,109,110,112,113,114,116,117,118,120,121,
  122,124,125,127,128,129,131,132,134,135,137,138,140,141,143,144,
  146,147,149,150,152,153,155,156,158,160,161,163,164,166,168,169,
  171,172,174,176,177,179,181,182,184,186,188,189,191,193,195,196,
  198,200,202,203,205,207,209,211,212,214,216,218,220,222,224,225,
  227,229,231,233,235,237,239,241,243,245,247,249,251,253,255,255,
};

void blink1_enableDegamma()
{
    blink1_enable_degamma = 1;
}
void blink1_disableDegamma()
{
    blink1_enable_degamma = 0;
}

// a simple logarithmic -> linear mapping as a sort of gamma correction
// maps from 0-255 to 0-255
static int blink1_degamma_log2lin( int n )  
{
  //return  (int)(1.0* (n * 0.707 ));  // 1/sqrt(2)
  return (((1<<(n/32))-1) + ((1<<(n/32))*((n%32)+1)+15)/32);
}

//
int blink1_degamma( int n ) 
{ 
    //return degamma_lookup[n];
    return blink1_degamma_log2lin(n);
}

// qsort C-string comparison function 
int cstring_cmp(const void *a, const void *b) 
{ 
    return strncmp( (const char *)a, (const char *)b, pathstrmax);
} 

//
void blink1_sortPaths(void)
{
    size_t elemsize = sizeof( blink1_cached_paths[0] ); // 128 
    //size_t count = sizeof(blink1_cached_paths) / elemsize; // 16
    
    return qsort( blink1_cached_paths, blink1_cached_count,elemsize,cstring_cmp);
}

//
void blink1_sortDevs(void)
{
    size_t elemsize = sizeof( blink1_cached_serials[0] ); //  
    //size_t count = sizeof(blink1_cached_serials) / elemsize; // 
    
    qsort(blink1_cached_serials,blink1_cached_count,elemsize,cstring_cmp);
}

//
int blink1_vid(void)
{
    uint8_t  rawVid[2] = {USB_CFG_VENDOR_ID};
    int vid = rawVid[0] + 256 * rawVid[1];
    return vid;
}
//
int blink1_pid(void)
{
    uint8_t  rawPid[2] = {USB_CFG_DEVICE_ID};
    int pid = rawPid[0] + 256 * rawPid[1];
    return pid;
}

//
char *blink1_error_msg(int errCode)
{
    /*
    static char buf[80];

    switch(errCode){
        case USBOPEN_ERR_ACCESS:    return "Access to device denied";
        case USBOPEN_ERR_NOTFOUND:  return "The specified device was not found";
        case USBOPEN_ERR_IO:        return "Communication error with device";
        default:
            sprintf(buf, "Unknown USB error %d", errCode);
            return buf;
    }
    */
    return NULL;    /* not reached */
}

/*
//
int blink1_command(hid_device* dev, int num_send, int num_recv,
                       uint8_t* buf_send, uint8_t* buf_recv )
{
    if( dev==NULL ) {
        return -1; // BLINK1_ERR_NOTOPEN;
    }
    int err = 0;
    if( (err = usbhidSetReport(dev, (char*)buf_send, num_send)) != 0) {
        fprintf(stderr,"error writing data: %s\n",blink1_error_msg(err));
        return err;
    }
     
    if( num_recv > 0 ) { 
        int len = num_recv;
        if((err = usbhidGetReport(dev, 0, (char*)buf_recv, &len)) != 0) {
            fprintf(stderr,"error reading data: %s\n",blink1_error_msg(err));
        } else {  // it was good
        }
    }
    return err;
}
*/