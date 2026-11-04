/*
 * GQmpeg
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * I must also give credit to those that
 * contributed all the code for non Linux platforms...
 * ...and apologize when I mangle it ;)
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


/* current systems with supported mixer functions:
 *   Linux
 *   the BSDs (FreeBSD, NetBSD, OpenBSD)
 *   SPARC Ultra 1 (tested with Solaris 2.5.1)
 *   SGI
 *   Tru64 (OSF1)
 *   HPUX (10.X & 11.X) - requires Makefile adjustments, see Note in HPUX section below
 *
 * Each platform interface now has it's own section.
 */

#include "gqmpeg.h"
#include "mixer.h"

#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef linux
#include <linux/soundcard.h>
#endif

#ifdef __FreeBSD__
#include <sys/soundcard.h>
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/audioio.h>
#endif

#if defined(sun) && defined(__svr4__)
#include <sys/audioio.h>
#endif

#ifdef __sgi
#include <math.h>
#include <audio.h>
#endif

#if defined(__osf__) && defined(HAVE_OSFMME)
#include <mme_api.h>
#endif

#ifdef __hpux
#include <Alib.h>
#endif

typedef struct _DeviceData DeviceData;
struct _DeviceData
{
	gint device_id;
	gchar *device_name;
	gint stereo;
	gint recordable;
};

static void mixer_set_vol(DeviceData *device, gint vol);
static gint mixer_get_vol(DeviceData *device);

static gint mixer_enabled = FALSE;
static gint current_vol = 0;
static gint current_bal = 50;

static GList *device_list = NULL;
static DeviceData *current_device = NULL;

/*
 *--------------------------------------------------------------------
 * Linux (and FreeBSD uses the same interface)
 *--------------------------------------------------------------------
 */

#if defined (linux) || defined (__FreeBSD__)

#define LEFT  1
#define RIGHT 256
#define LEFT_MASK 0x00ff
#define RIGHT_MASK 0xff00

void mixer_init(gint init_device_id)
{
	char *device_names[] = SOUND_DEVICE_NAMES;
	int dev_mask = 0;
	int rec_mask = 0;
	int stereo_mask = 0;
	char *mixer_device;
	int i;
	int mf = -1;
	int t;

	mixer_device = getenv("MIXERDEVICE");
	if (mixer_device == NULL) mixer_device = "/dev/mixer";

	mf = open(mixer_device, O_RDWR);
	if(mf < 0)
		{
		printf(_("unable to open %s\n"), mixer_device);
		mixer_enabled = FALSE;
		return;
		}

	t = ioctl(mf, SOUND_MIXER_READ_DEVMASK, &dev_mask);
	t|= ioctl(mf, SOUND_MIXER_READ_RECMASK, &rec_mask);
	t|= ioctl(mf, SOUND_MIXER_READ_STEREODEVS, &stereo_mask);
	if (t!=0)
		{
		printf(_("Unable to determine mixer devices\n"));
		close(mf);
		mixer_enabled = FALSE;
		return;
		}

	current_device = NULL;

	/* get device listing */
	for (i=0; i<SOUND_MIXER_NRDEVICES; i++)
		{
		if ( (dev_mask & (1<<i)) ) /* skip unsupported devices */
 			{
			DeviceData *device = g_new0(DeviceData, 1);
			device->device_id = i;
			device->device_name = device_names[i];
			device->stereo = (stereo_mask & (1<<i));
			device->recordable = (rec_mask & (1<<i));
			device_list = g_list_append(device_list, device);

			if (debug_mode) printf("Mixer device added to list: %d, %s, %d, %d\n",
					device->device_id, device->device_name, device->stereo, device->recordable);

			if (init_device_id == i) current_device = device;
			}
		}

	close (mf);

	if (device_list)
		{
		mixer_enabled = TRUE;
		if (!current_device) current_device = device_list->data;
		current_vol = mixer_get_vol(current_device);
		}
	else
		{
		mixer_enabled = FALSE;
		}
}

static void mixer_set_vol(DeviceData *device, gint vol)
{
	int mf = -1;
	gint l, r;
	gint new_vol = 0x0000;
	char *mixer_device;

	if (!mixer_enabled) return;
	if (!device) return;

	if (current_bal == 50)
		{
		l = vol;
		r = vol;
		}
	else if (current_bal > 50)
		{
		l = (float)vol * (100 - current_bal) / 50;
		r = vol;
		}
	else
		{
		l = vol;
		r = (float)vol * current_bal / 50;
		}

	new_vol = l * LEFT + r * RIGHT;

	mixer_device = getenv("MIXERDEVICE");
	if (mixer_device == NULL) mixer_device = "/dev/mixer";

	mf = open(mixer_device, O_RDWR);
	if(mf < 0)
		{
		printf(_("unable to open %s\n"), mixer_device);
		return;
		}

	if(ioctl(mf, MIXER_WRITE(device->device_id), &new_vol)<0)
		{
		printf(_("failed to set volume on %d\n"), device->device_id);
		}

	close(mf);

	if (debug_mode) printf("volume set to %d,%d\n", l, r);
}

static gint mixer_get_vol(DeviceData *device)
{
	int mf = -1;
	gint vol = 0;
	gint l;
	gint r;
	char *mixer_device;

	if (!mixer_enabled) return 0;
	if (!device) return 0;

	mixer_device = getenv("MIXERDEVICE");
	if (mixer_device == NULL) mixer_device = "/dev/mixer";

	mf = open(mixer_device, O_RDWR);
	if(mf < 0)
		{
		printf(_("unable to open %s\n"), mixer_device);
		return 0;
		}

	if(ioctl(mf, MIXER_READ(device->device_id), &vol)<0)
		{
		printf(_("failed to get volume on %d\n"), device->device_id);
		vol = 0;
		}

	close(mf);

	l = vol & LEFT_MASK;
	r = (vol & RIGHT_MASK) >> 8;

	if (l == r)
		{
		current_bal = 50;
		}
	else if (l<r)
		{
		current_bal = 50 + (float)(r - l) / r * 50;
		}
	else
		{
		current_bal = 50 - (float)(l - r) / l * 50;
		}

	if (debug_mode) printf("volume is %d,%d\n", l, r);

	if (l > r)
		{
		return l;
		}
	else
		{
		return r;
		}
}

/*
 *--------------------------------------------------------------------
 * NetBSD and OpenBSD
 *--------------------------------------------------------------------
 */

#elif defined(__NetBSD__) || defined(__OpenBSD__)

mixer_devinfo_t *infos;
mixer_ctrl_t *values;

void mixer_init(gint init_device_id)
{
  int fd, i, ndev;
  char *mixer_device;
  audio_device_t adev;
  mixer_devinfo_t dinfo;

  mixer_device = getenv("MIXERDEVICE");
  if (mixer_device == NULL)
    mixer_device = "/dev/mixer0";

  if ((fd = open(mixer_device, O_RDWR)) == -1) {
    perror(mixer_device);
    mixer_enabled = FALSE;
  }

  if (ioctl(fd, AUDIO_GETDEV, &adev) == -1) {
    perror(mixer_device);
    close(fd);
    mixer_enabled = FALSE;
  }

  for (ndev = 0; ; ndev++) {
    dinfo.index = ndev;
    if (ioctl(fd, AUDIO_MIXER_DEVINFO, &dinfo) == -1)
      break;
  }
  infos = calloc(ndev, sizeof *infos);
  values = calloc(ndev, sizeof *values);

  for (i = 0; i < ndev; i++) {
    infos[i].index = i;
    ioctl(fd, AUDIO_MIXER_DEVINFO, &infos[i]);
  }

  for (i = 0; i < ndev; i++) {
    values[i].dev = i;
    values[i].type = infos[i].type;
    if (infos[i].type == AUDIO_MIXER_VALUE) {
      DeviceData *device = g_new0(DeviceData, 1);
      device->device_id = i;
	/* if problem, revert back to this:
      device->device_name = infos[i].label.name;
	*/
      if (0 <= infos[i].mixer_class && infos[i].mixer_class < ndev)
       device->device_name = g_strconcat(infos[infos[i].mixer_class].label.name, "." ,infos[i].label.name, NULL);
      else
       device->device_name = infos[i].label.name;
      device->stereo = 1;
      device->recordable = 0;
      device_list = g_list_append(device_list, device);

      values[i].un.value.num_channels = 2;
      if (ioctl(fd, AUDIO_MIXER_READ, &values[i]) == -1) {
        values[i].un.value.num_channels = 1;
        if (ioctl(fd, AUDIO_MIXER_READ, &values[i]) == -1)
          perror("AUDIO_MIXER_READ");
      }

      if (debug_mode) printf("Mixer device added to list: %d, %s, %d\n",
                             device->device_id, device->device_name,
                             device->stereo);
      if (init_device_id == i) current_device = device;
    }
  }

  close(fd);

  if (device_list) {
    mixer_enabled = TRUE;
    if (!current_device)
      current_device = device_list->data;
    current_vol = mixer_get_vol(current_device);
  } else {
    mixer_enabled = FALSE;
  }
}

static void mixer_set_vol(DeviceData *device, gint vol)
{
  int fd;
  char *mixer_device;
  mixer_ctrl_t *m;

  mixer_device = getenv("MIXERDEVICE");
  if (mixer_device == NULL)
    mixer_device = "/dev/mixer0";

  if ((fd = open(mixer_device, O_RDWR)) == -1) {
    perror(mixer_device);
    close(fd);
  }

  m = &values[device->device_id];
  if (ioctl(fd, AUDIO_MIXER_WRITE, m) == -1) {
    perror("AUDIO_MIXER_WRITE");
    close(fd);
  }

  close(fd);

  if (m->un.value.num_channels == 2) {
    /* input and output seem to only have one channel?? */
    if (device->device_id == 13 || device->device_id == 14) {
      m->un.value.level[0] = vol * AUDIO_MAX_GAIN / 100;
    } else if (current_bal < 50) {
      m->un.value.level[0] = vol * AUDIO_MAX_GAIN / 100;
      m->un.value.level[1] = m->un.value.level[0] * current_bal / 50;
    } else if (current_bal > 50) {
      m->un.value.level[1] = vol * AUDIO_MAX_GAIN / 100;
      m->un.value.level[0] = m->un.value.level[1] * (100 - current_bal) / 50;
    } else {
      m->un.value.level[0] = m->un.value.level[1] = vol * AUDIO_MAX_GAIN / 100;
    }
  } else {
    m->un.value.level[0] = vol * AUDIO_MAX_GAIN / 100;
  }
  /* from AUDIO_MIN_GAIN (0) to AUDIO_MAX_GAIN (255) */

  if (debug_mode) printf("volume set to %d (%d)\n", vol, current_bal);
}

static gint mixer_get_vol(DeviceData *device)
{
  int fd;
  char *mixer_device;
  mixer_ctrl_t *m;

  mixer_device = getenv("MIXERDEVICE");
  if (mixer_device == NULL)
    mixer_device = "/dev/mixer0";

  if ((fd = open(mixer_device, O_RDWR)) == -1) {
    perror(mixer_device);
    close(fd);
    return -1;
  }

  m = &values[device->device_id];
  if (ioctl(fd, AUDIO_MIXER_READ, m) == -1) {
    perror("AUDIO_MIXER_READ");
    close(fd);
    return -1;
  }

  close(fd);

  if (m->un.value.num_channels == 2) {
    if (m->un.value.level[0] > m->un.value.level[1]) {
      current_bal = m->un.value.level[1] * 50 / m->un.value.level[0];
      return m->un.value.level[0] * 100 / AUDIO_MAX_GAIN;
    } else if (m->un.value.level[0] < m->un.value.level[1]) {
      current_bal = 100 - (m->un.value.level[0] * 50 / m->un.value.level[1]);
      return m->un.value.level[1] * 100 / AUDIO_MAX_GAIN;
    } else {
      current_bal = 50;
      return m->un.value.level[0] * 100 / AUDIO_MAX_GAIN;
    }
  } else {
    current_bal = 50;
    return m->un.value.level[0] * 100 / AUDIO_MAX_GAIN;
  }
}

/*
 *--------------------------------------------------------------------
 * Sun (svr4)
 *--------------------------------------------------------------------
 */

#elif defined(sun) && defined(__svr4__)

static int device_ids[] = { AUDIO_SPEAKER,
			    AUDIO_LINE_OUT,
			    AUDIO_HEADPHONE
};
void mixer_init(gint init_device_id)
{
  char *device_names[] = { "speaker",
			   "lineout",
			   "headphone"
  };  
  const char *default_device = "/dev/audioctl";
  int afd = -1;
  int mode = O_RDONLY;
  audio_device_t adev;
  const char *device = default_device;
  int i;
  
  if ((afd = open(device, mode)) == -1) {
    perror(device);
    mixer_enabled = FALSE;
    return ;
  }
  
  if (ioctl(afd, AUDIO_GETDEV, &adev) == -1) {
    perror(device);
    close(afd);
    mixer_enabled = FALSE;
    return;
  }
  close(afd);

  for (i=0; i < 3; i++) {
    DeviceData *device = g_new0(DeviceData, 1);
    device->device_id = i;
    device->device_name = device_names[i];
    device->stereo = 1;
    device->recordable = 0;
    device_list = g_list_append(device_list, device);
    
    if (debug_mode) printf("Mixer device added to list: %d, %s, %d\n",
			   device->device_id, device->device_name,
			   device->stereo);   
    if (init_device_id == i) current_device = device;
  }
  
  if (device_list) {
    mixer_enabled = TRUE;
    if (!current_device)
      current_device = device_list->data;
    current_vol = mixer_get_vol(current_device);
  } else {
    mixer_enabled = FALSE;
  }
}

static void mixer_set_vol(DeviceData *device, gint vol)
{
  const char *default_device = "/dev/audioctl";  /* see comment for previous /dev/audioctl */
  int afd = -1;
  int mode = O_RDONLY;
  audio_device_t adev;
  
  if ((afd = open(default_device, mode)) == -1) {
    perror(default_device);
    mixer_enabled = FALSE;
    return ;
  }
  
  if (ioctl(afd, AUDIO_GETDEV, &adev) == -1) {
    perror(default_device);
    close(afd);
    mixer_enabled = FALSE;
    return;
  }

  {    
    int port;
    audio_info_t ainfo;
    
    AUDIO_INITINFO(&ainfo);
    port = device_ids[device->device_id];
    
    /* SUN: from AUDIO_MIN_GAIN (0) to AUDIO_MAX_GAIN (255) */
    ainfo.play.gain = vol * AUDIO_MAX_GAIN / 100;
    ainfo.play.port = port;
    
    /* SUN: from AUDIO_LEFT_BALANCE (0) to AUDIO_RIGHT_BALANCE (64) */
    ainfo.play.balance = current_bal * AUDIO_RIGHT_BALANCE / 100;
        
    if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
      perror("AUDIO_SETINFO 1");
      close(afd);
      return;
    }
    if (debug_mode) {
      printf("volume set to %d (%d)\n", vol, current_bal);
    }
  }
  close(afd);
  
  return;
}

static gint mixer_get_vol(DeviceData *device)
{
  const char *default_device = "/dev/audio";  
  int afd = -1;
  int mode = O_RDONLY;
  audio_device_t adev;
  audio_info_t ainfo;
  
  if ((afd = open(default_device, mode)) == -1) {
    perror(default_device);
    mixer_enabled = FALSE;
    return -1;
  }
  
  if (ioctl(afd, AUDIO_GETDEV, &adev) == -1) {
    perror(default_device);
    close(afd);
    mixer_enabled = FALSE;
    return -1;
  }
    
  /*  AUDIO_INITINFO(&ainfo); */
  
  if (ioctl(afd, AUDIO_GETINFO, &ainfo) == -1) {
    perror("AUDIO_GETINFO");
    close(afd);
    return -1;
  }

  if (ainfo.play.balance > AUDIO_RIGHT_BALANCE)
    current_bal = 50;
  else
    current_bal = ainfo.play.balance * 100 / AUDIO_RIGHT_BALANCE;
  
  close(afd);
  
  return ainfo.play.gain * 100 / AUDIO_MAX_GAIN; 
}

/*
 *--------------------------------------------------------------------
 * SGI
 *--------------------------------------------------------------------
 */

#elif defined(__sgi)

long volconv[101];

void mixer_init(gint init_device_id)
{
	float conv;
	int i;
	DeviceData *device = g_new0(DeviceData, 1);
	device->device_id = AL_DEFAULT_DEVICE;
	device->device_name = "Default Audio";
	device->stereo = 1;
        device->recordable = 0;
	device_list = g_list_append(device_list, device);

	for( i=0; i<101; i++ )
	{
		conv = i;
		conv /= 100;
		conv *= conv * conv; /* scale = percent * percent * percent */
		volconv[i] = (long)(conv*255);
	}

	current_vol = mixer_get_vol( device );

	mixer_enabled = TRUE;
}

static void mixer_set_vol(DeviceData *device, gint vol)
{
/*
	long	volset[4];

	volset[0] = AL_LEFT_SPEAKER_GAIN;
	volset[2] = AL_RIGHT_SPEAKER_GAIN;

	if (vol < 0) vol = 0;
	if (vol > 100) vol = 100;

	if ( 50 != current_bal )
	{
		float	lpcnt, rpcnt, balpcnt;

		lpcnt = ((float)vol)/100.F;
		rpcnt = lpcnt;

		balpcnt = ((float)(50 - current_bal))/50.F;
		lpcnt *= (1.F - balpcnt);
		rpcnt *= (1.F + balpcnt);

		volset[1] = volconv[(long)(lpcnt*100.F)];
		volset[3] = volconv[(long)(rpcnt*100.F)];
	}
	else
	{
		volset[1] = volconv[vol];
		volset[3] = volconv[vol];
	}

	ALsetparams( AL_DEFAULT_DEVICE, volset, 4 );
*/
        /* added by cht73c on 12/27/00 - new audio library support */
        ALpv pvs[1];
        ALfixed gain[8]; /* for gain. 8 is the max # of channels we'll suppt */
        ALparamInfo pinfo; /* for getting maximum/minimum gain */
        gint mindB, maxdB, numChannels;
	gint i;

        /* the SGIs now have the concept of multi-channel sound devices.
         * All sound-level information is recorded in decibels (dB) and
         * since each device might have any number of channels and any
         * maximum/minimum gain (amplification), we must ask for each.
         * If there are more than two output channels, the "set_vol" function
         * takes a guess as to what L & R are and sets them.
         * Balance means "how much energy is directed
         * away from the left channel".  Therefore balance=80 means 20%left,
         * 80%right. We will use a different methedology here.  If the balance
         * is not 50, then then <40 channel gets a percentage appropriate
         * for the current volume level.  
         * example: balance=30. volume=50% of max.  L=70, R=30.  
         *       therefore, left=50%, right=(50%)*(30/50)=30%
         * In short, balance only diminishes a channel.  Previous algorithm
         * would have amplified the left channel.
        */
        /* get the min/max gain in db */
        if ( alGetParamInfo(AL_DEFAULT_OUTPUT, AL_GAIN, &pinfo) < 0 ) {
           printf("alGetParamInfo failed: %s\n",alGetErrorString(oserror()));
	   return ;
        }
        mindB = alFixedToInt(pinfo.min.ll); /* don't assume this is zero */
        maxdB = alFixedToInt(pinfo.max.ll); /* assume this is gt mindB */
        
        /* get the number of channels on the default device */
        pvs[0].param = AL_CHANNELS; /* a vector parameter */
        if (alGetParams(AL_DEFAULT_OUTPUT, pvs, 1 ) < 0 ) {
           printf("alGetParams failed: %s\n",alGetErrorString(oserror()));
	   return ;
        }
        numChannels=(int)pvs[0].value.i; /* ALpv.ALvalue structures */
	gain[0]=alIntToFixed((int) ((vol*0.01)*(maxdB-mindB))+mindB);
	switch (numChannels) {
	   case 1:
	         /* do nothing */
		 break;
	   case 2:
                 /* assume L=gain[0], R=gain[1] */
		 gain[1]=gain[0]; /* initial is equal */
                 if (current_bal < 50) { /* reduce right volume */
		    gain[1]=alIntToFixed((int)
                          ( (float)alFixedToInt(gain[1]) * 
			     ( (float)current_bal/50.0) 
			  ) 
		       );
		 } else if (current_bal > 50) {
		    gain[0]=alIntToFixed((int)
                          ( (float)alFixedToInt(gain[0]) * 
			     ( (float)(100-current_bal)/50.0) 
			  ) 
		       );
		 }
		 break;
	   case 3:
                 /* assume gain[1] is the center channel. Set it to vol,
                  * regardless of balance */
		 gain[1]=gain[0]; /* center channel set to the volume */
		 gain[2]=gain[0];
                 if (current_bal < 50) { /* reduce right volume */
		    gain[2]=alIntToFixed((int)
                          ( (float)alFixedToInt(gain[2]) * 
			     ( (float)current_bal/50.0) 
			  ) 
		       );
		 } else if (current_bal > 50) {
		    gain[0]=alIntToFixed((int)
                          ( (float)alFixedToInt(gain[0]) * 
			     ( (float)(100-current_bal)/50.0) 
			  ) 
		       );
		 }
		 break;
	   default:
                 /* assume that gain[0] & gain[1] are L and R. Set the
                  * remainder to vol*/
		 gain[1]=gain[0];
		 for (i=2; i<numChannels; i++) {
		    gain[i]=gain[0]; /* non-(left or right) channel */
		 }
                 if (current_bal < 50) { /* reduce right volume */
		    gain[1]=alIntToFixed((int)
                          ( (float)alFixedToInt(gain[1]) * 
			     ( (float)current_bal/50.0) 
			  ) 
		       );
		 } else if (current_bal > 50) {
		    gain[0]=alIntToFixed((int)
                          ( (float)alFixedToInt(gain[0]) * 
			     ( (float)(100-current_bal)/50.0) 
			  ) 
		       );
		 }
	}

        pvs[0].param=AL_GAIN;
        pvs[0].value.ptr=gain;
        pvs[0].sizeIn=numChannels;
        if (alSetParams(AL_DEFAULT_OUTPUT, pvs, 1 ) < 0 ) {
           printf("alSetParams failed: %s\n",alGetErrorString(oserror()));
        }

	return;
}

/* modified the global variable current_bal which is the current balance */
static gint mixer_get_vol(DeviceData *device)
{
	gint retvol = 0;
	float lpcnt, rpcnt;
        /* added by cht73c on 12/27/00 - new audio library support */
        ALpv pvs[1];
        ALfixed gain[8]; /* for gain. 8 is the max # of channels we'll suppt */
        ALparamInfo pinfo; /* for getting maximum/minimum gain */
        gint mindB, maxdB, numChannels;
	gint counter;

        /* the SGIs now have the concept of multi-channel sound devices.
         * All sound-level information is recorded in decibels (dB) and
         * since each device might have any number of channels and any
         * maximum/minimum gain (amplification), we must ask for each.
         * If there are more than two output channels, the "get_vol" function
         * returns the average volume. 
         * Sometimes, the gain returned from alGetParams will be negative
         * infinity.  In these cases, the returned value will be clamped
         * to mindB. */
        /* get the min/max gain in db */
        if ( alGetParamInfo(AL_DEFAULT_OUTPUT, AL_GAIN, &pinfo) < 0 ) {
           printf("alGetParamInfo failed: %s\n",alGetErrorString(oserror()));
	   return mixer_enabled;
        }
        mindB = alFixedToInt(pinfo.min.ll); /* don't assume this is zero */
        maxdB = alFixedToInt(pinfo.max.ll); /* assume this is gt mindB */
        
        /* get the gain for each channel on the default device */
        pvs[0].param = AL_GAIN; /* a vector parameter */
        pvs[0].value.ptr = gain; /* the vector we've allocated */
        pvs[0].sizeIn = 8; /* the number of elements in gain */
        if (alGetParams(AL_DEFAULT_OUTPUT, pvs, 1 ) < 0 ) {
           printf("alGetParams failed: %s\n",alGetErrorString(oserror()));
	   return mixer_enabled;
        }
        numChannels=pvs[0].sizeOut;
        for (counter=0; counter < numChannels; counter++) {
	   if (alFixedToInt(gain[counter]) < mindB ) { /* clamp to mindB */
	       gain[counter]=alIntToFixed(mindB);
	   }
	}
	switch (numChannels) {
	   case 1:
		 current_bal=50;
		 lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		      1.F/3.F);
		 retvol = (gint)(100.*lpcnt);
		 break;
	   case 2:
		 if (gain[0] == gain[1]) {
	   	    current_bal=50;
		    lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    retvol = (gint)(100.*lpcnt);
		 } else {
		    lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    rpcnt=powf(alFixedToDouble(gain[1])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    current_bal = 50 - 50 * ( lpcnt - rpcnt );
		    retvol = (gint)( 100. * ( (lpcnt+rpcnt) * 0.5 ) );
		 }
		 break;
	   case 3:
                 /* assume that gain[1] is the center channel. skip it */
		 if (gain[0] == gain[1] && gain[0] == gain[2]) {
	   	    current_bal=50;
		    lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    retvol = (gint)(100.*lpcnt);
		 } else {
		    lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    rpcnt=powf(alFixedToDouble(gain[2])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    current_bal = 50 - 50 * ( lpcnt - rpcnt );
		    retvol = (gint)( 100. * ( (lpcnt+rpcnt) * 0.5 ) );
		 }
		 break;
	   case 4:
                 /* assume that gain[0] & gain[1] are L and R. skip the rest */
		 if (gain[0] == gain[1] ) {
	   	    current_bal=50;
		    lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    retvol = (gint)(100.*lpcnt);
		 } else {
		    lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    rpcnt=powf(alFixedToDouble(gain[1])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    current_bal = 50 - 50 * ( lpcnt - rpcnt );
		    retvol = (gint)( 100. * ( (lpcnt+rpcnt) * 0.5 ) );
                 }
		 break;
	   default:
                 /* assume that gain[0] & gain[1] are L and R. skip the rest */
		 if (gain[0] == gain[1] ) {
	   	    current_bal=50;
		    lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    retvol = (gint)(100.*lpcnt);
		 } else {
		    lpcnt=powf(alFixedToDouble(gain[0])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    rpcnt=powf(alFixedToDouble(gain[1])/(float)(maxdB-mindB),\
		         1.F/3.F);
		    current_bal = 50 - 50 * ( lpcnt - rpcnt );
		    retvol = (gint)( 100. * ( (lpcnt+rpcnt) * 0.5 ) );
                 }
	}
	return retvol;
}

/*
 *--------------------------------------------------------------------
 * Tru64 (OSF1)
 *--------------------------------------------------------------------
 */

#elif defined(__osf__) && defined(HAVE_OSFMME)

#define AUDIO_MAX_GAIN  0x0000FFFF
#define LEFT_OFFSET             16
#define RIGHT_OFFSET             0

void mixer_init(gint init_device_id)
{
	int i;
	LPWAVEOUTCAPS waveOutCaps_p = NULL;
	UINT max_indev  = waveInGetNumDevs();
	UINT max_outdev = waveOutGetNumDevs();

	if ((max_indev <= 0) && (max_outdev <= 0))
		{
		printf(_("no OSF MME devices found\n"));
		mixer_enabled = FALSE;
		return;
		}

	waveOutCaps_p = (WAVEOUTCAPS *)mmeAllocMem(sizeof(WAVEOUTCAPS));

	for (i = 0; i < max_outdev || i < max_indev; ++i)
		{
		DeviceData *device = g_new0(DeviceData, 1);
		device->device_id = i;
		device->device_name = NULL; /*= device_names[i];*/

		/* may be overwritten below */
		device->recordable = 0;
		device->stereo = 0;

		if (i < max_outdev)
			{
			waveOutGetDevCaps(i, waveOutCaps_p, sizeof(WAVEOUTCAPS));
			device->stereo = (waveOutCaps_p->dwSupport & WAVECAPS_LRVOLUME);
			}
		if (i < max_indev)
			{
			device->recordable = 1;
			}

		device_list = g_list_append(device_list, device);

		if (debug_mode)
			{
			printf("Mixer device added to list: %d, %s, %d, %d\n",
			       device->device_id, device->device_name,
			       device->stereo, device->recordable);
			}

		if (init_device_id == i) current_device = device;
		}

	mmeFreeMem((char *)waveOutCaps_p);

	if (device_list)
		{
		mixer_enabled = TRUE;
		if (!current_device) current_device = device_list->data;
		current_vol = mixer_get_vol(current_device);
		}
	else
		{
		mixer_enabled = FALSE;
		}
}

static void mixer_set_vol(DeviceData *device, gint vol)
{
	long l, r;
	DWORD new_vol;

	if (!mixer_enabled) return;

	if (current_bal == 50)
		{
		l = vol;
		r = vol;
		}
	else if (current_bal > 50)
		{
		l = (float)vol * (100 - current_bal) / 50;
		r = vol;
		}
	else
		{
		l = vol;
		r = (float)vol * current_bal / 50;
		}

	l = l * AUDIO_MAX_GAIN / 100;
	r = r * AUDIO_MAX_GAIN / 100;
	new_vol = (l << LEFT_OFFSET) | (r << RIGHT_OFFSET);

	waveOutSetVolume(device->device_id, new_vol);

	if (debug_mode)
		{
		printf("volume set to %3d (0x%04x, 0x%04x = 0x%08x)\n",
		       vol, l, r, new_vol);
		}
}

static gint mixer_get_vol(DeviceData *device)
{
	long l=0, r=0;
	DWORD *vol_p = NULL;

	if (!mixer_enabled) return 0;

	/* get the volume while thrashing the heap */
	vol_p = (LPDWORD) (mmeAllocMem(sizeof (DWORD)));
	waveOutGetVolume(device->device_id, vol_p);

	l = ((*vol_p >> LEFT_OFFSET) & AUDIO_MAX_GAIN) * 100 / AUDIO_MAX_GAIN;
	r = ((*vol_p >> RIGHT_OFFSET)& AUDIO_MAX_GAIN) * 100 / AUDIO_MAX_GAIN;

	if (l == r)
		{
		current_bal = 50;
		}
	else if (l<r)
		{
		current_bal = 50 + (float)(r - l) / r * 50;
		}
	else
		{
		current_bal = 50 - (float)(l - r) / l * 50;
		}

	if (debug_mode)
		{
		printf("volume is (0x%08x = 0x%04x, 0x%04x)\n", *vol_p, l, r);
		}

	mmeFreeMem((char *)vol_p);

	if (l > r)
		{
		return l;
		}
	else
		{
		return r;
		}
}

/*
 *--------------------------------------------------------------------
 * HPUX (10.X & 11.X)
 *--------------------------------------------------------------------
 */

#elif defined(__hpux)

/* Note: this requires additions the the Makefile:
 * add an include path to /opt/audio/include and a libpath
 * /opt/audio/lib and link with lib libAlib.a.
 *
 * TODO: someone should automate this in configure.in...
 */

void mixer_init(gint init_device_id)
{
  Audio *ad = NULL;
  AGainDB       min;
  AGainDB       max;
  AGainDB       cur;
  DeviceData *device;
  long  ret;
  
  if ((ad = AOpenAudio(NULL, &ret)) == NULL) {
    perror("Audio");
    mixer_enabled = FALSE;
    return ;
  }
  
  max = AMaxOutputGain(ad);
  min = AMinOutputGain(ad);

  ACloseAudio(ad, &ret);

  device = g_new0(DeviceData, 1);
  device->device_id = 1;
  device->device_name = "Default Audio";
  device->stereo = 1;
  device_list = g_list_append(device_list, device);

    
  if (debug_mode) printf("Mixer device added to list: %d, %s, %d\n",
                          device->device_id, device->device_name,
                          device->stereo);   
  current_device = device;
  
  current_vol = mixer_get_vol(current_device);
}

static void mixer_set_vol(DeviceData *device, gint vol)
{
  Audio *ad = NULL;
  AGainDB       min;
  AGainDB       max;
  gint          lvol, rvol;
  AGainDB       lcur, rcur;
  long  ret;
  
  if ((ad = AOpenAudio(NULL, &ret)) == NULL) {
    perror("Audio");
    mixer_enabled = FALSE;
    return ;
  }
  
  max = AMaxOutputGain(ad);
  min = AMinOutputGain(ad);


  if (current_bal < 50) {
    lvol = vol;
    rvol = lvol * current_bal / 50;
  } else if (current_bal > 50) {
    rvol = vol;
    lvol = rvol * (100 - current_bal) / 50;
  } else {
    lvol = rvol = vol;
  }

  lcur =  min + (AGainDB)(((float)(max - min)) * lvol/100.0);
  rcur =  min + (AGainDB)(((float)(max - min)) * rvol/100.0);

  ASetSystemChannelGain(ad, ASGTPlay, ACTLeft, lcur, &ret);
  ASetSystemChannelGain(ad, ASGTPlay, ACTRight, rcur, &ret);

  ACloseAudio(ad, &ret);

  return;
}

static gint mixer_get_vol(DeviceData *device)
{
  Audio *ad = NULL;
  AGainDB       min;
  AGainDB       max;
  gint          lvol, rvol;
  AGainDB       lcur, rcur;
  long  ret;
  
  if ((ad = AOpenAudio(NULL, &ret)) == NULL) {
    perror("Audio");
    mixer_enabled = FALSE;
    return ;
  }
  
  max = AMaxOutputGain(ad);
  min = AMinOutputGain(ad);

  AGetSystemChannelGain(ad, ASGTPlay, ACTLeft, &lcur, &ret);
  AGetSystemChannelGain(ad, ASGTPlay, ACTRight, &rcur, &ret);

  ACloseAudio(ad, &ret);

  lvol = ((float)(lcur - min)) / ((float)(max - min))*100.0;
  rvol = ((float)(rcur - min)) / ((float)(max - min))*100.0;

  if (lvol > rvol) {
    current_bal = rvol * 50 / lvol;
    return lvol;
  } else if (lvol < rvol) {
    current_bal = 100 - (lvol * 50 / rvol);
    return rvol;
  } else {
    current_bal = 50;
    return lvol;
  }

  return;
}

/*
 *--------------------------------------------------------------------
 * Empty functions for unsupported platforms
 *--------------------------------------------------------------------
 */

#else

void mixer_init(gint init_device_id)
{
	mixer_enabled = FALSE;
}

static void mixer_set_vol(DeviceData *device, gint vol)
{
	return;
}

static gint mixer_get_vol(DeviceData *device)
{
	return 0;
}

#endif

/*
 *--------------------------------------------------------------------
 * these are the functions that the app should call
 *--------------------------------------------------------------------
 */

/* do _not_ free the data contained in the list, only the list */
GList *get_mixer_device_list(void)
{
	GList *list = NULL;
	GList *work = device_list;

	while(work)
		{
		DeviceData *device = work->data;
		list = g_list_append(list, device->device_name);
		work = work->next;
		}
	return list;
}

gint get_mixer_device_id(gchar *device_name)
{
	GList *list = device_list;
	if (!device_name) return -1;

	while(list)
		{
		DeviceData *device = list->data;
		if (strcmp(device->device_name, device_name) == 0) return device->device_id;
		list = list->next;
		}

	return -1;
}

void set_mixer_device(gint device_id)
{
	GList *list = device_list;

	while(list)
		{
		DeviceData *device = list->data;
		if (device->device_id == device_id)
			{
			current_device = device;
			mixer_get_vol(device);
			return;
			}
		list = list->next;
		}
}

/* 0 through 100 */
void set_volume(gint vol)
{
	if (vol < 0) vol = 0;
	if (vol > 100) vol = 100;

	if (current_device && !current_device->stereo)  current_bal = 50;

	mixer_set_vol(current_device, vol);

	current_vol = vol;
}

gint get_volume(void)
{
/*
 * This should aways be called,
 * but some platforms did not have it update the volume (mixer_get_vol),
 * and I am not going to mess with it.
 */
#if defined (linux) || defined (__FreeBSD__)
	current_vol = mixer_get_vol(current_device);
#endif

#if defined(__osf__) && defined(HAVE_OSFMME)
	current_vol = mixer_get_vol(current_device);
#endif

	return current_vol;
}

/* from 0 through 100: 0 left, 50 center, 100 right */
void set_balance(gint bal)
{  
	
	if (current_device && !current_device->stereo)
		{
		current_bal = 50;
		return;
		}
	
	if (bal < 0) bal = 0;
	if (bal > 100) bal = 100;

	current_bal = bal;
	
	if (debug_mode) printf("balance set to %d\n", bal);

	mixer_set_vol(current_device, current_vol);
}

gint get_balance(void)
{
	return current_bal;
}


