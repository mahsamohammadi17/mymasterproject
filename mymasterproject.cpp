/*
 *  server_example_goose.c
 *
 *  This example demonstrates how to use GOOSE publishing, Reporting and the
 *  control model.
 *
 */


#include "iec61850_server.h"
#include "hal_thread.h" /* for Thread_sleep() */
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include<error.h>
#include <bits/stdc++.h>
#include "static_model.h"
#include "sv_subscriber.h"
#include "goose_receiver.h"
#include "goose_subscriber.h"
#include "goose_publisher.h"

#define pass return
using namespace std;

/* import IEC 61850 device model created from SCL-File */
extern IedModel iedModel;

static int running = 0;
static IedServer iedServer = NULL;

void sigint_handler(int signalId)
{
    running = 0;
}

struct fun_para_g
{
    char * interface;
 //   uint16_t appid;

};

struct GOOSEvaluestatus//CTRL
{
    __int32_t XCBR_Pos;
    __int32_t XSWI_Pos,XSWI_Pos2;
    __int32_t PTRC_EEHealth;
    bool XCBR_Loc;
}gostatus,gostatusold;

struct GOOSEvaluealarm//PROT
{
    bool PIOC_Op_general;
    __int32_t XCBR_EEHealth;
    bool LPHD_PwrSupAlm;
    bool PSCH_ProTx,PSCH_ProRx;
}goalarm,goalarmold;
struct GOOSEvaluemeas//MEAS
{
    float aphsa,aphsb,aphsc;
    float phva,phvb,phvc;
    float totw,totvar;
    float hz,totpf;
}gomeas,gomeasold;
#define MMXU_A_phsA_instCVal_mag_f__ aphsa
#define MMXU_A_phsB_instCVal_mag_f__ aphsb
#define MMXU_A_phsC_instCVal_mag_f__ aphsc
#define MMXU_PhV_phsA_instCVal_mag_f__ phva
#define MMXU_PhV_phsB_instCVal_mag_f__ phvb
#define MMXU_PhV_phsC_instCVal_mag_f__ phvc
#define MMXU_TotW_instMag_f__ totw
#define MMXU_TotVAr_instMag_f__ totvar
#define MMXU_Hz_instMag_f__ hz
#define MMXU_TotPF_instMag_f__ totpf
MmsValue * mmsvaluestatus, *mmsvaluealarm, *mmsvaluemeas;
void gooseListener(GooseSubscriber subscriber, void* parameter)
{
    uint16_t *appid;
    appid = (uint16_t *)parameter;
    printf("appid: %d\n",*appid);
    printf("GOOSE event:\n");
    printf("  stNum: %u sqNum: %u\n", GooseSubscriber_getStNum(subscriber),
           GooseSubscriber_getSqNum(subscriber));
    printf("  timeToLive: %u\n", GooseSubscriber_getTimeAllowedToLive(subscriber));

    uint64_t timestamp = GooseSubscriber_getTimestamp(subscriber);

    printf("  timestamp: %u.%u\n", (uint32_t) (timestamp / 1000), (uint32_t) (timestamp % 1000));

    MmsValue* values = GooseSubscriber_getDataSetValues(subscriber);

    char buffer[1024];

    MmsValue_printToBuffer(values, buffer, 1024);
    printf("%s\n\n", buffer);
    switch( *appid){
        case 901: mmsvaluestatus=MmsValue_clone(values);break;
        case 902: mmsvaluealarm=MmsValue_clone(values);break;
        case 903: mmsvaluemeas=MmsValue_clone(values);break;
        default: break;
    }
}

#define subscribeGOOSEfromrealIEDstatus subscribeGOOSEfromrealIED
void* subscribeGOOSEfromrealIED(void *arg)
{
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    char * interface=para.interface;

    GooseReceiver receiver = GooseReceiver_create();//GooseReceiver receiver2 = GooseReceiver_create();GooseReceiver receiver3= GooseReceiver_create();
 /*   if (argc > 1) {
        printf("Set interface id: %s\n", argv[1]);
        GooseReceiver_setInterfaceId(receiver, argv[1]);
    }
    else {*/
        printf("Goose receiver: Using interface enp0s8\n");
        GooseReceiver_setInterfaceId(receiver, "enp0s8");//GooseReceiver_setInterfaceId(receiver2, "enp0s8");GooseReceiver_setInterfaceId(receiver3, "enp0s8");
  //  }

    GooseSubscriber subscriber = GooseSubscriber_create("LIED20CTRL/LLN0$GO$Status", NULL);
 //   GooseSubscriber subscriber2 = GooseSubscriber_create("LIED20PROT/LLN0$GO$Alarm", NULL);
 //   GooseSubscriber subscriber3 = GooseSubscriber_create("LIED20MEAS/LLN0$GO$Meas", NULL);
  //  void *ch=NULL;
    uint16_t appid=901;uint16_t *appidp;appidp=&appid;
    GooseSubscriber_setAppId(subscriber, appid); //GooseSubscriber_setAppId(subscriber2, 902); GooseSubscriber_setAppId(subscriber3, 903);
    GooseSubscriber_setListener(subscriber, gooseListener, (void*)appidp);//GooseSubscriber_setListener(subscriber2, gooseListener, NULL);GooseSubscriber_setListener(subscriber3, gooseListener, NULL);
    GooseReceiver_addSubscriber(receiver, subscriber);//GooseReceiver_addSubscriber(receiver2, subscriber);GooseReceiver_addSubscriber(receiver3, subscriber);
    GooseReceiver_start(receiver);//GooseReceiver_start(receiver2);GooseReceiver_start(receiver3);
    signal(SIGINT, sigint_handler);
    while (running) {
        Thread_sleep(3);
        if (mmsvaluestatus!=NULL){
        try
        {
          //  printf("%s\n", MmsValue_getOctetStringBuffer(mmsvaluestatus));
            char buffer[1024];
            MmsValue_printToBuffer(mmsvaluestatus, buffer, 1024);
            string buffers=buffer;
       //     cout<<"string: "<<buffers<<endl;
            buffers=buffers.substr(1,buffers.length()-2);
            memset(buffer,'\0',sizeof(buffer));
            strcpy(buffer,buffers.c_str());
        //    printf("buffer: %s\n",buffer);
            char *delim=",";
            char *p;
            gostatus.XCBR_Pos= atoi(strtok(buffer,delim));
            gostatus.XSWI_Pos= atoi(strtok(NULL,delim));gostatus.XSWI_Pos2=atoi(strtok(NULL,delim));
            gostatus.PTRC_EEHealth= atoi(strtok(NULL,delim));
            gostatus.XCBR_Loc= strcmp(strtok(NULL,delim),"true")==0;
          //  cout<<gostatus.XCBR_Pos<< gostatus.XSWI_Pos<<gostatus.XSWI_Pos2<< gostatus.PTRC_EEHealth<<boolalpha<<gostatus.XCBR_Loc<<endl;

        }
        catch (...)
        {
           printf("*********************************\n");
           continue;
        }
        }

    }
    GooseReceiver_stop(receiver);//GooseReceiver_stop(receiver2);GooseReceiver_stop(receiver3);
    GooseReceiver_destroy(receiver);//GooseReceiver_destroy(receiver2);GooseReceiver_destroy(receiver3);

}
void* subscribeGOOSEfromrealIEDalarm(void *arg){
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    char * interface=para.interface;
    GooseReceiver receiver2 = GooseReceiver_create();//GooseReceiver receiver3= GooseReceiver_create();
    printf("Goose receiver: Using interface enp0s8\n");
    GooseReceiver_setInterfaceId(receiver2, "enp0s8");//GooseReceiver_setInterfaceId(receiver3, "enp0s8");
    GooseSubscriber subscriber2 = GooseSubscriber_create("LIED20PROT/LLN0$GO$Alarm", NULL);
    // GooseSubscriber subscriber3 = GooseSubscriber_create("LIED20MEAS/LLN0$GO$Meas", NULL);
    uint16_t appid=902;uint16_t *appidp;appidp=&appid;
    GooseSubscriber_setAppId(subscriber2, appid); //GooseSubscriber_setAppId(subscriber3, 903);
    GooseSubscriber_setListener(subscriber2, gooseListener, (void*)appidp);//GooseSubscriber_setListener(subscriber3, gooseListener, NULL);
    GooseReceiver_addSubscriber(receiver2, subscriber2);//GooseReceiver_addSubscriber(receiver3, subscriber);
    GooseReceiver_start(receiver2);//GooseReceiver_start(receiver3);
    signal(SIGINT, sigint_handler);
    while (running) {
        Thread_sleep(3);
        if (mmsvaluealarm!=NULL){
            try
            {
                char buffer[1024];
                MmsValue_printToBuffer(mmsvaluealarm, buffer, 1024);
                string buffers=buffer;
          //      cout<<"string: "<<buffers<<endl;
                buffers=buffers.substr(1,buffers.length()-2);
                memset(buffer,'\0',sizeof(buffer));
                strcpy(buffer,buffers.c_str());
         //       printf("buffer: %s\n",buffer);
                char *delim=",";
                char *p;
                goalarm.PIOC_Op_general= strcmp(strtok(buffer,delim),"true")==0;
                goalarm.XCBR_EEHealth= atoi(strtok(NULL,delim));
                goalarm.LPHD_PwrSupAlm=strcmp(strtok(NULL,delim),"true")==0;
                goalarm.PSCH_ProTx= strcmp(strtok(NULL,delim),"true")==0;
                goalarm.PSCH_ProRx= strcmp(strtok(NULL,delim),"true")==0;
            //    cout<<boolalpha<<goalarm.PIOC_Op_general<<noboolalpha<< goalarm.XCBR_EEHealth<<boolalpha<< goalarm.LPHD_PwrSupAlm<< goalarm.PSCH_ProTx<<goalarm.PSCH_ProRx<<endl;
            }
            catch (...)
            {
                printf("111122222222222222222222112\n");
                continue;
            }
        }
    }
    GooseReceiver_stop(receiver2);//GooseReceiver_stop(receiver3);
    GooseReceiver_destroy(receiver2);//GooseReceiver_destroy(receiver3);

};

void* subscribeGOOSEfromrealIEDmeas(void *arg){
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    char * interface=para.interface;
    GooseReceiver receiver3= GooseReceiver_create();
    printf("Goose receiver: Using interface enp0s8\n");
    GooseReceiver_setInterfaceId(receiver3, "enp0s8");
    GooseSubscriber subscriber3 = GooseSubscriber_create("LIED20MEAS/LLN0$GO$Meas", NULL);
    uint16_t appid=903;uint16_t *appidp;appidp=&appid;
    GooseSubscriber_setAppId(subscriber3, appid);
    GooseSubscriber_setListener(subscriber3, gooseListener, (void*)appidp);
    GooseReceiver_addSubscriber(receiver3, subscriber3);
    GooseReceiver_start(receiver3);
    signal(SIGINT, sigint_handler);
    while (running) {
        Thread_sleep(3);
        if (mmsvaluemeas!=NULL){
            try
            {
                char buffer[1024];
                MmsValue_printToBuffer(mmsvaluemeas, buffer, 1024);
                string buffers=buffer;
           //     cout<<"string: "<<buffers<<endl;
                buffers=buffers.substr(1,buffers.length()-2);
                memset(buffer,'\0',sizeof(buffer));
                strcpy(buffer,buffers.c_str());
         //       printf("buffer: %s\n",buffer);
                char *delim=",";
                char *p;
                gomeas.aphsa= (float)atof(strtok(buffer,delim));gomeas.aphsb=(float) atof(strtok(NULL,delim));gomeas.aphsc=(float)atof(strtok(NULL,delim));
                gomeas.phva= (float)atof(strtok(NULL,delim));gomeas.phvb=(float)atof(strtok(NULL,delim));gomeas.phvc=(float)atof(strtok(NULL,delim));
                gomeas.totw=(float)atof(strtok(NULL,delim));
                gomeas.totvar=(float)atof(strtok(NULL,delim));
                gomeas.hz=(float)atof(strtok(NULL,delim));
                gomeas.totpf=(float)atof(strtok(NULL,delim));
              //  cout<<gomeas.aphsa<<gomeas.aphsb<< gomeas.aphsc<<gomeas.phva<< gomeas.phvb<< gomeas.phvc<<gomeas.totw<<gomeas.totvar<<gomeas.hz<< gomeas.totpf<<endl;
            }
            catch (...)
            {
                printf("3333333333333333333333333333333333\n");
                continue;
            }
        }
    }
    GooseReceiver_stop(receiver3);
    GooseReceiver_destroy(receiver3);

};
/* Callback handler for received SV messages */
static void svUpdateListener (SVSubscriber subscriber, void* parameter, SVSubscriber_ASDU asdu)
{
    printf("svUpdateListener called\n");

    const char* svID = SVSubscriber_ASDU_getSvId(asdu);

    if (svID != NULL)
        printf("  svID=(%s)\n", svID);

    printf("  smpCnt: %i\n", SVSubscriber_ASDU_getSmpCnt(asdu));
    printf("  confRev: %u\n", SVSubscriber_ASDU_getConfRev(asdu));

    /*
     * Access to the data requires a priori knowledge of the data set.
     * For this example we assume a data set consisting of FLOAT32 values.
     * A FLOAT32 value is encoded as 4 bytes. You can find the first FLOAT32
     * value at byte position 0, the second value at byte position 4, the third
     * value at byte position 8, and so on.
     *
     * To prevent damages due configuration, please check the length of the
     * data block of the SV message before accessing the data.
     */
    if (1==1){//(SVSubscriber_ASDU_getDataSize(asdu) >= 8) {
        printf("   DATA[0]: %f\n", SVSubscriber_ASDU_getFLOAT32(asdu, 0));
        printf("   DATA[1]: %f\n", SVSubscriber_ASDU_getFLOAT32(asdu, 4));
        printf("   DATA[2]: %f\n", SVSubscriber_ASDU_getFLOAT32(asdu, 4*2));
        printf("   DATA[3]: %f\n", SVSubscriber_ASDU_getFLOAT32(asdu, 4*3));
        printf("   DATA[4]: %f\n", SVSubscriber_ASDU_getFLOAT32(asdu, 4*4));
        printf("   DATA[5]: %f\n", SVSubscriber_ASDU_getFLOAT32(asdu, 4*5));
    }
}


void* subscribeSV(void *arg)
{
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    char * interface=para.interface;
    SVReceiver receiver = SVReceiver_create();

  /*  if (argc > 1) {
        SVReceiver_setInterfaceId(receiver, argv[1]);
        printf("Set interface id: %s\n", argv[1]);
    }*/
  //  else {
        printf("SV subscriber: Using interface enp0s8\n");
        SVReceiver_setInterfaceId(receiver, "enp0s8");
  //  }

    /* Create a subscriber listening to SV messages with APPID 4000h */
    SVSubscriber subscriber = SVSubscriber_create(NULL, 0x4000);

    /* Install a callback handler for the subscriber */
    SVSubscriber_setListener(subscriber, svUpdateListener, NULL);

    /* Connect the subscriber to the receiver */
    SVReceiver_addSubscriber(receiver, subscriber);

    /* Start listening to SV messages - starts a new receiver background thread */
    SVReceiver_start(receiver);

    signal(SIGINT, sigint_handler);

    while (running)
        Thread_sleep(2);

    /* Stop listening to SV messages */
    SVReceiver_stop(receiver);

    /* Cleanup and free resources */
    SVReceiver_destroy(receiver);

}



void controlHandlerForBinaryOutput(void* parameter, MmsValue* value)
{
    uint64_t timestamp = Hal_getTimeInMs();


/*    if (parameter ==  IEDMODEL_CTRL_LLN0_Beh ) {//(parameter == IEDMODEL_CTRL_LLN0) {
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_LLN0_Beh_t, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_CTRL_LLN0_Beh_stVal, value);
    }*/

/*    if (parameter == IEDMODEL_CTRL_SPDOesGGIO2_SPCSO) {
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_SPDOesGGIO2_SPCSO_t, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_CTRL_SPDOesGGIO2_SPCSO_stVal, value);
    }*/
/*
    if (parameter == IEDMODEL_CTRL_SPDOesGGIO3_SPCSO) {
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_SPDOesGGIO3_SPCSO_t, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_CTRL_SPDOesGGIO3_SPCSO_stVal, value);
    }

    if (parameter == IEDMODEL_CTRL_SPDOesGGIO4_SPCSO) {
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_SPDOesGGIO4_SPCSO_t, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_CTRL_SPDOesGGIO4_SPCSO_stVal, value);
    }

    if (parameter==IEDMODEL_CTRL_SPDOnsGGIO1_SPCSO){
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_SPDOnsGGIO1_SPCSO_t, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_CTRL_SPDOnsGGIO1_SPCSO_stVal, value);
    }

    if (parameter==IEDMODEL_CTRL_SPDOnsGGIO2_SPCSO){
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_SPDOnsGGIO2_SPCSO_t, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_CTRL_SPDOnsGGIO2_SPCSO_stVal, value);
    }

    if (parameter==IEDMODEL_CTRL_SPDOnsGGIO3_SPCSO){
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_SPDOnsGGIO3_SPCSO_t, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_CTRL_SPDOnsGGIO3_SPCSO_stVal, value);
    }
    if (parameter==IEDMODEL_CTRL_SPDOnsGGIO4_SPCSO){
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_SPDOnsGGIO4_SPCSO_t, timestamp);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_CTRL_SPDOnsGGIO4_SPCSO_stVal, value);
    }*/
}

void logic_inside(void* arg){
    pass;
}

void give_alarm(void* arg){
    pass;
}

void* goosepublisherMAIN(void *arg){

    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;

    iedServer = IedServer_create(&iedModel);
//	IedServer_

//	if (argc > 1) {
    char* ethernetIfcID = para.interface;
    ethernetIfcID="enp0s8";
    printf("GOOSE publisher: Using GOOSE interface: %s\n", ethernetIfcID);
    IedServer_setGooseInterfaceId(iedServer, ethernetIfcID);
    //}

    /* MMS server will be instructed to start listening to client connections. */
    IedServer_start(iedServer, 1103);


    //IedServer_setControlHandler(iedServer, IEDMODEL_CTRL_LLN0, (ControlHandler) controlHandlerForBinaryOutput,    IEDMODEL_CTRL_LLN0);

    /*IedServer_setControlHandler(iedServer, IEDMODEL_CTRL_SPDOesGGIO1_SPCSO, (ControlHandler) controlHandlerForBinaryOutput,
                                IEDMODEL_CTRL_SPDOesGGIO1_SPCSO);

    IedServer_setControlHandler(iedServer, IEDMODEL_CTRL_SPDOesGGIO2_SPCSO, (ControlHandler) controlHandlerForBinaryOutput,
                                IEDMODEL_CTRL_SPDOesGGIO2_SPCSO);

    IedServer_setControlHandler(iedServer, IEDMODEL_CTRL_SPDOesGGIO3_SPCSO, (ControlHandler) controlHandlerForBinaryOutput,
                                IEDMODEL_CTRL_SPDOesGGIO3_SPCSO);*/

    if (!IedServer_isRunning(iedServer)) {
        printf("Starting server failed! Exit.\n");
        IedServer_destroy(iedServer);
        exit(-1);
    }

    /* Start GOOSE publishing */
    IedServer_enableGoosePublishing(iedServer);


    signal(SIGINT, sigint_handler);

    float anIn1=0.1f;
    while (running) {
        IedServer_lockDataModel(iedServer);
        /*       Timestamp ts;
               Timestamp_clearFlags(&ts);
               Timestamp_setTimeInMilliseconds(&ts, Hal_getTimeInMs());*/
#define update_GOOSE_via_realIED 1
#ifdef     update_GOOSE_via_realIED
        /*UPDATE STATUS*/
        if(IedServer_getInt32AttributeValue(iedServer,IEDMODEL_CTRL_XCBR_Pos_stVal)!=gostatus.XCBR_Pos){
             IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal, gostatus.XCBR_Pos);
        }
        if(IedServer_getInt32AttributeValue(iedServer,IEDMODEL_CTRL_XSWI_Pos_stVal)!=gostatus.XSWI_Pos){
            IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XSWI_Pos_stVal, gostatus.XSWI_Pos);
        }
        if(IedServer_getInt32AttributeValue(iedServer,IEDMODEL_CTRL_PTRC_EEHealth_stVal)!=gostatus.PTRC_EEHealth){
            IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_PTRC_EEHealth_stVal, gostatus.PTRC_EEHealth);
        }
        if(IedServer_getBooleanAttributeValue(iedServer,IEDMODEL_CTRL_XCBR_Loc_stVal)!=gostatus.XCBR_Loc){
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Loc_stVal, gostatus.XCBR_Loc);
        }
        /*UPDATE ALARM*/
        if(IedServer_getBooleanAttributeValue(iedServer,IEDMODEL_PROT_PIOC_Op_general)!=goalarm.PIOC_Op_general){
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_general, goalarm.PIOC_Op_general);
            IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_PROT_PIOC_Op_t,Hal_getTimeInMs());
        }
        if(IedServer_getInt32AttributeValue(iedServer,IEDMODEL_PROT_XCBR_EEHealth_stVal)!=goalarm.XCBR_EEHealth){
            IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_PROT_XCBR_EEHealth_stVal,goalarm.XCBR_EEHealth);
        }
        if(IedServer_getBooleanAttributeValue(iedServer,IEDMODEL_PROT_LPHD_PwrSupAlm_stVal)!=goalarm.LPHD_PwrSupAlm){
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_LPHD_PwrSupAlm_stVal, goalarm.LPHD_PwrSupAlm);
        }
        if(IedServer_getBooleanAttributeValue(iedServer,IEDMODEL_PROT_PSCH_ProTx_stVal)!=goalarm.PSCH_ProTx){
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PSCH_ProTx_stVal, goalarm.PSCH_ProTx);
        }
        if(IedServer_getBooleanAttributeValue(iedServer,IEDMODEL_PROT_PSCH_ProRx_stVal)!=goalarm.PSCH_ProRx){
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PSCH_ProRx_stVal,goalarm.PSCH_ProRx);
        }
        /*UPDATE MEASurement*/
        if(IedServer_getFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_A_phsA_instCVal_mag_f)!=gomeas.MMXU_A_phsA_instCVal_mag_f__){
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsA_instCVal_mag_f,gomeas.MMXU_A_phsA_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_A_phsA_t,Hal_getTimeInMs());
        }
        if(IedServer_getFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_A_phsB_instCVal_mag_f)!=gomeas.MMXU_A_phsB_instCVal_mag_f__){
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsB_instCVal_mag_f,gomeas.MMXU_A_phsB_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_A_phsB_t,Hal_getTimeInMs());
        }
        if(IedServer_getFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_A_phsC_instCVal_mag_f)!=gomeas.MMXU_A_phsC_instCVal_mag_f__){
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsC_instCVal_mag_f,gomeas.MMXU_A_phsC_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_A_phsC_t,Hal_getTimeInMs());
        }//**********************************************
        if(IedServer_getFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_PhV_phsA_instCVal_mag_f)!=gomeas.MMXU_PhV_phsA_instCVal_mag_f__){
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsA_instCVal_mag_f,gomeas.MMXU_PhV_phsA_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_PhV_phsA_t,Hal_getTimeInMs());
        }
        if(IedServer_getFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_PhV_phsB_instCVal_mag_f)!=gomeas.MMXU_PhV_phsB_instCVal_mag_f__){
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsB_instCVal_mag_f,gomeas.MMXU_PhV_phsB_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_PhV_phsB_t,Hal_getTimeInMs());
        }
        if(IedServer_getFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_PhV_phsC_instCVal_mag_f)!=gomeas.MMXU_PhV_phsC_instCVal_mag_f__){
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsC_instCVal_mag_f,gomeas.MMXU_PhV_phsC_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_PhV_phsC_t,Hal_getTimeInMs());
        }//**********************************************
        if(IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotW_instMag_f)!=gomeas.totw){
            IedServer_updateFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_TotW_instMag_f, gomeas.totw );
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotW_t,Hal_getTimeInMs() );
        }
        if(IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotVAr_instMag_f)!=gomeas.totvar){
            IedServer_updateFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_TotVAr_instMag_f, gomeas.totvar );
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotVAr_t,Hal_getTimeInMs() );
        }
        if(IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_instMag_f)!=gomeas.hz){
            IedServer_updateFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_Hz_instMag_f, gomeas.hz);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_t,Hal_getTimeInMs() );
        }
        if(IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotPF_instMag_f)!=gomeas.totpf){
            IedServer_updateFloatAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_TotPF_instMag_f, gomeas.totpf  );
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotPF_t,Hal_getTimeInMs() );
        }/*UPDATE FINISHED*/
#endif

        //      IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_t, Hal_getTimeInMs());
    //    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_instMag_f, anIn1);

#ifdef gpincreasestnum
        GoosePublisher_increaseStNum(publisher);
#endif
        IedServer_unlockDataModel(iedServer);

        Thread_sleep(3);
    }

    /* stop MMS server - close TCP server socket and all client sockets */
    IedServer_stop(iedServer);
    /* Cleanup - free all resources */
    IedServer_destroy(iedServer);
}


int main(int argc, char** argv) {
     mmsvaluestatus=NULL; mmsvaluealarm=NULL;mmsvaluemeas=NULL;
    running = 1;
    pthread_t thread[10];
    int t_id[10];
    struct fun_para_g parag[10];
    for (int i =0;i<10;i++)parag[i].interface="enp0s8";
  //  parag[0].interface="enp0s8";parag[1].interface="enp0s8";parag[2].interface="enp0s8";parag[3].interface="enp0s8";parag[4].interface="enp0s8";

    t_id [ 0 ] = pthread_create(&thread[0], NULL , goosepublisherMAIN , & parag [ 0 ] ) ;
    t_id [ 1 ] = pthread_create(&thread[1], NULL , subscribeGOOSEfromrealIED , & parag [ 1 ] ) ;
    t_id [ 2 ] = pthread_create(&thread[2], NULL , subscribeSV , & parag [ 2 ] ) ;
    t_id [ 3 ] = pthread_create(&thread[3], NULL , subscribeGOOSEfromrealIEDalarm , & parag [ 3 ] ) ;
    t_id [ 4 ] = pthread_create(&thread[4], NULL , subscribeGOOSEfromrealIEDmeas , & parag [ 4 ] ) ;

    pthread_join(thread[0], NULL ) ;
    pthread_join(thread[1], NULL ) ;
    pthread_join(thread[2], NULL ) ; pthread_join(thread[3], NULL ) ; pthread_join(thread[4], NULL ) ;
//  subscribeGOOSEfromrealIED(&parag[1]);


    return 0;
} /* main() */




//   IedServer_updateTimestampAttributeValue(iedServer, IEDMODEL_MEAS_LLN0_Mod_t,&ts);

/*  IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_CTRL_LLN0_Beh_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_CTRL_LLN0_Health_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_CTRL_LLN0_LEDRs_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_CTRL_LLN0_Loc_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_CTRL_LLN0_Mod_t,Hal_getTimeInMs());

  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_PROT_LLN0_Beh_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_PROT_LLN0_Health_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_PROT_LLN0_Mod_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_PROT_LLN0_OpTmh_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_PROT_PIOC_Op_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_PROT_PTOC_Op_t,Hal_getTimeInMs());

  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_LLN0_Beh_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_LLN0_Health_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_LLN0_Mod_t,Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_Mod_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_Beh_t, Hal_getTimeInMs());////
  IedServer_updateUTCTimeAttributeValue(iedServer,      IEDMODEL_MEAS_MMXU_TotW_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,       IEDMODEL_MEAS_MMXU_TotVAr_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,       IEDMODEL_MEAS_MMXU_TotVA_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,    IEDMODEL_MEAS_MMXU_TotPF_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,     IEDMODEL_MEAS_MMXU_Hz_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,   IEDMODEL_MEAS_MMXU_PPV_phsAB_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,     IEDMODEL_MEAS_MMXU_PPV_phsBC_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_PPV_phsCA_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_PhV_phsA_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_PhV_phsB_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_PhV_phsC_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_PhV_res_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_A_phsA_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_A_phsB_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_A_phsC_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_A_neut_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_A_res_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_W_phsA_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_W_phsB_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_W_phsC_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_VAr_phsA_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_VAr_phsB_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_VAr_phsC_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_VA_phsA_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_VA_phsB_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,IEDMODEL_MEAS_MMXU_VA_phsC_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_PF_phsA_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,        IEDMODEL_MEAS_MMXU_PF_phsB_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,    IEDMODEL_MEAS_MMXU_PF_phsC_t, Hal_getTimeInMs());
  IedServer_updateUTCTimeAttributeValue(iedServer,  IEDMODEL_MEAS_MMXU_Health_t, Hal_getTimeInMs());*/




//printf("%ld\n",Hal_getTimeInMs());
//   IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn1_mag_f, anIn1);
