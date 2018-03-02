/*
  PiArduI2C

  Usage: ./PiArduI2C

*/

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <sys/stat.h>
#include <map>
#include <list>
#include <chrono>
#include <thread>

using namespace std;

char DataPath[] = "/home/pi/data/";
char TProbePath[] = "/home/pi/data/tprobe/";
char RelayPath[] = "/home/pi/data/relay/";

map<string, list<time_t> > TProbeTimeStamps;
time_t StartTimeStamp;

enum DataId
{
   DataId_WSPT1=1,
   DataId_DS18x20=2,
};

struct WSPT1_DATA
{
   int16_t start;
   int16_t temperature;
   int16_t id;
   char checksum;
};

struct DS18x20_DATA
{
   char ROM[8];
   char data[10];
   int16_t temperature;
};

struct DATA
{
   char dataId;
   char buffer[28];
};

/**
 * calcul la qualité des mesures recus (pourcentage)
 * pendant la période glissante periodSecond
 * depuis la date de démarrage StartTimeStamp
 * avec les timestamps de reception tstamps
 * sachant que durant periodSecond nbMeasure doivent être recues

*/
unsigned int computeQuality(unsigned int periodSecond, time_t startTimeStamp
, list<time_t>& tstamps, time_t currentTime, unsigned int nbMeasure )
{
   // au début on ne tient pas compte des valeurs qui ont pu être mémorisées avant
   if(currentTime-startTimeStamp<30) return 0;
 
   unsigned int quality = 0;
   time_t startTime = currentTime - periodSecond;
   if( startTimeStamp > startTime )
      startTime = startTimeStamp;
   
   tstamps.insert(tstamps.end(), currentTime );
   
   // supprime les timestamps plus vieux que la periode
   list<time_t>::iterator it = tstamps.begin();
   while((*it)<startTime) it = tstamps.erase(it);

   // ajuste le nombre de mesures en fonction de la période réelle
   nbMeasure = (unsigned int)(static_cast<double>(nbMeasure) * ( static_cast<double>(currentTime - startTime) / periodSecond ));
   if(nbMeasure==0) nbMeasure = 1;
 
    printf("nbMeasure[%d] currentTime[%d] startTime[%d] size[%d]\n"
    , nbMeasure, currentTime, startTime, tstamps.size());
    
   if( nbMeasure > 0)
      quality = (unsigned int) ((100.0*tstamps.size()) / nbMeasure);
   return quality;
}

int main(int argc, char *argv[])
{
   struct stat st = {0};
   if (stat(DataPath, &st) == -1)
   {
      mkdir(DataPath, 0755);
   }
   if (stat(TProbePath, &st) == -1)
   {
      mkdir(TProbePath, 0755);
   }
   if (stat(RelayPath, &st) == -1)
   {
      mkdir(RelayPath, 0755);
   }

   time( &StartTimeStamp );

   FILE* fp;
   char relayValues[8];
   for(unsigned int i=0;i<8;++i)
   {
      relayValues[i]=2;
   }

   int slaveI2CId = 8;
   char i2Cdevice[] = "/dev/i2c-1";
   int fd = ::open(i2Cdevice, O_RDWR);
   if (fd < 0)
   {
      printf("No I2C (%s)\n", i2Cdevice);
      return 1;
   }
   if (ioctl (fd, I2C_SLAVE, slaveI2CId) < 0)
   {
      printf("Cannot find slave %d\n", slaveI2CId);
      ::close(fd);
      return 2;
   }

   const char CmdRelay=0x34;
   while(fd>=0)
   {
      // relais
      for(unsigned int i=0;i<=7;++i)
      {
         char relayValue;
         char filename[256];
         sprintf(filename,"%s/R%d", RelayPath, i+1);
         fp = fopen(filename,"r");
         if(fp!=NULL)
         {
            char line[10];
            char* res=fgets(line,10,fp);
            if(res)
            {
               relayValue = atoi(res);
               if(relayValues[i]!=relayValue)
               {
                  printf("relay[%d]=%d\n", i+1, relayValue);
                  relayValues[i]=relayValue;
                  const unsigned char cmd[3] = {CmdRelay, static_cast<char>(i+1), relayValue };
                  int nbWrite = i2c_smbus_write_i2c_block_data(fd, 0, 3, cmd);
               }
            }
            fclose(fp);
         }
      }
      DATA data;
      unsigned char* raw = (unsigned char*)&data;
      int nbRead = i2c_smbus_read_i2c_block_data(fd,0,sizeof(DATA),raw);
      printf("nbRead=%d/%d\n", nbRead, sizeof(DATA));
      if(nbRead==sizeof(DATA))
      {
         printf("%X%08X%08X%08X%08X%08X%08X%08X %08X\n",(char)*raw
         ,*((long*)raw+1),*((long*)raw+5),*((long*)raw+9)
         ,*((long*)raw+13),*((long*)raw+17),*((long*)raw+21)
         ,*((long*)raw+25),*((long*)raw+29));
         char line[256];
         time_t rawtime;
         time ( &rawtime );
         struct tm *timeinfo = localtime ( &rawtime );
         char dateF[256];
         strftime(dateF,256,"%H:%M:%S",timeinfo);
         switch(data.dataId)
         {
            case 0:
               break;
            case DataId_WSPT1:
            {
               WSPT1_DATA* wspt1 = (WSPT1_DATA*)data.buffer;

               list<time_t>& tstamps = TProbeTimeStamps["WSPT1"];
               unsigned int quality = computeQuality(3600, StartTimeStamp, tstamps, rawtime, 60 );

               sprintf(line,"date[%s] type[WSPT1] temp[%.2f] start[%08d] id[%08d] chk[%02d] quality[%d]\n"
                  , dateF, (float)wspt1->temperature/10.0, wspt1->start, wspt1->id
                  , wspt1->checksum, quality);
               printf(line);
               char filename[256];
               sprintf(filename,"%s/WSPT1", TProbePath);
               fp = fopen(filename,"w");
               if(fp!=NULL)
               {
                  fprintf(fp,line);
                  fclose(fp);
               }
               sprintf(filename,"%s/WSPT1_log", TProbePath);
               fp = fopen(filename,"a+");
               if(fp!=NULL)
               {
                  fprintf(fp,line);
                  fclose(fp);
               }
            }
            break;
            case DataId_DS18x20:
            {
               DS18x20_DATA* ds18 = (DS18x20_DATA*)data.buffer;
               char ROM[256];
               char* pos = ROM;
               sprintf(pos,"%02X-",ds18->ROM[7]);
               pos+=3;
               for( unsigned int i = 6; i >0; --i)
               {
                  sprintf(pos,"%02x",ds18->ROM[i]);
                  pos+=2;
               }
               char sData[256];
               pos = sData;
               for( unsigned int i = 0; i < 8; ++i)
               {
                  if(i>0) sprintf(pos++," ");
                  sprintf(pos,"%02X",ds18->data[i]);
                  pos+=2;
               }

               list<time_t>& tstamps = TProbeTimeStamps[ROM];
               unsigned int quality = computeQuality(3600, StartTimeStamp, tstamps, rawtime, 120 );

               sprintf(line,"date[%s] type[DS18x20] temp[%.2f] ROM[%s] data[%s] quality[%d]\n"
                  , dateF, (float)ds18->temperature/100.0, ROM, sData, quality);
               printf(line);
               char filename[256];
               sprintf(filename,"%s/%s", TProbePath, ROM);
               fp = fopen(filename,"w");
               if(fp!=NULL)
               {
                  fprintf(fp,line);
                  fclose(fp);
               }
            }
            break;
            default:
               printf("Unknwown id[%d]\n", data.dataId);
         }
      }
      this_thread::sleep_for(chrono::milliseconds(100));
   }
}
