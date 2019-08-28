/*
 *  server_example_goose.c
 *
 *  This example demonstrates how to use GOOSE publishing, Reporting and the
 *  control model.
 *
 */
//fgwfrz2p776vxqkm.onion

#include "iec61850_server.h"
#include "hal_thread.h" /* for Thread_sleep() */
//#include <signal.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <pthread.h>
//#include <errno.h>
//#include<error.h>
#include <bits/stdc++.h>
#include "static_model.h"
#include "sv_subscriber.h"
#include "goose_receiver.h"
#include "goose_subscriber.h"
#include "goose_publisher.h"

#define pass return
#define do_nothing ;
using namespace std;
#include "shared_parameters.cpp"
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
    int portnumber;
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

#define pi M_PI

MmsValue * mmsvaluestatus, *mmsvaluealarm, *mmsvaluemeas;
uint32_t stnum901, stnum902,stnum903,    stnum901last, stnum902last, stnum903last;
float VA,VB,VC,IA,IB,IC;
float datasv[10];

void gooseListener(GooseSubscriber subscriber, void* parameter)
{
    uint16_t *appid;
    appid = (uint16_t *)parameter;
    printf("appid: %d\n",*appid);
    printf("GOOSE event:\n");
    printf("  stNum: %u sqNum: %u\n", GooseSubscriber_getStNum(subscriber),
           GooseSubscriber_getSqNum(subscriber));
    if(*appid==901)stnum901=GooseSubscriber_getStNum(subscriber);
    else if(*appid==902)stnum902=GooseSubscriber_getStNum(subscriber);
    else if (*appid==903)stnum903=GooseSubscriber_getStNum(subscriber);
    else do_nothing
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

int give_alarm_overcurrent(void* arg);
int busbar_protection(void* arg){
    if(give_alarm_overcurrent(nullptr)==1) {
        cout<< "***\tbusbar_protection\t***\"***\\tbusbar_protection\\t***\"***\\tbusbar_protection\\t***\"***\\tbusbar_protection\\t***\"***\\tbusbar_protection\\t***"<< endl;
        return 1;
    }
    return 0;
}

float thresholdA=10.5*sqrt(2);
int breaker_failure_protection(void* arg){
    if ( (IA>thresholdA || IB >thresholdA || IC > thresholdA )&& (gostatus.XCBR_Pos==1 || goalarm.PIOC_Op_general!=true )){
        IedServer_updateInt32AttributeValue(iedServer,IEDMODEL_PROT_XCBR_EEHealth_stVal,3);
        IedServer_updateInt32AttributeValue(iedServer,IEDMODEL_CTRL_PTRC_EEHealth_stVal,3);
               give_alarm_overcurrent(nullptr); //?
        cout<<"***\tbreaker_failure_protection\t******\tbreaker_failure_protection\t******\tbreaker_failure_protection\t******\tbreaker_failure_protection\t******\tbreaker_failure_protection\t***"<<endl;
        return 1;
    }
    return 0;
}

int underfrequency_load_shedding(void* arg){
    if (datasv[6]<10){
        Thread_sleep(500);
        if (datasv[6]<10){
            IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal,0);//XCBR circuit breaker
            cout<<"***\tunderfrequency_load_shedding\t******\tunderfrequency_load_shedding\t******\tunderfrequency_load_shedding\t******\tunderfrequency_load_shedding\t******\tunderfrequency_load_shedding\t***"<<endl;
        return 1;
        }
    }
    return 0;
}

int check_status_for_XCBR_closed(void* arg){
    if( fabs(datasv[6] -50)<5 && IA<=thresholdA && IB <=thresholdA && IC <= thresholdA ){
        IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_general,false);
        if(IedServer_getInt32AttributeValue(iedServer,IEDMODEL_PROT_PIOC_Op_q)!=0)IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_t,Hal_getTimeInMs() );
        IedServer_updateQuality(iedServer, IEDMODEL_PROT_PIOC_Op_q,0);
        if( IedServer_getInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal)==0)IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal,1);//XCBR circuit breaker 0 open 1 closed
        IedServer_updateInt32AttributeValue(iedServer,IEDMODEL_PROT_XCBR_EEHealth_stVal,1);
        IedServer_updateInt32AttributeValue(iedServer,IEDMODEL_CTRL_PTRC_EEHealth_stVal,1);
        cout<<"##****\tXCBR_closed\t######****\tXCBR_closed\t######****\tXCBR_closed\t######****\tXCBR_closed\t######****\tXCBR_closed\t######****\tXCBR_closed\t######****\tXCBR_closed\t####"<<endl;
        return 1;
    }
    return 0;
}

void monitor_other_IEDs_for_status(void* arg){  // not used
    IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal, gostatus.XCBR_Pos);
   // IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XSWI_Pos_stVal, gostatus.XSWI_Pos);
    //IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_PTRC_EEHealth_stVal,gostatus.PTRC_EEHealth);
    IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Loc_stVal, gostatus.XCBR_Loc);
    IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_general,goalarm.PIOC_Op_general);
    IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_t, Hal_getTimeInMs());
    IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_PROT_XCBR_EEHealth_stVal,goalarm.XCBR_EEHealth);
    /**/        IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_LPHD_PwrSupAlm_stVal,goalarm.LPHD_PwrSupAlm);
    /**/        IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PSCH_ProTx_stVal, goalarm.PSCH_ProTx);
    /**/        IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PSCH_ProRx_stVal, goalarm.PSCH_ProRx);
   // cout<<"***\t\tmonitor_other_IEDs_for_status\t\t***"<<endl;
}

deque <float> aa,ab,ac,va,vb,vc;
int update_MEAS_from_SV(void* arg){
    /* Here I force to define 50Hz as the frequency */
#define update_MEASurement_via_SV 1
#if update_MEASurement_via_SV == 1
//#define HZ 50
    int queuelength=100;
    float PhVa,Aa,PhVb, Ab, PhVc, Ac;
    Aa=datasv[1]; PhVa=datasv[0]; Ab=datasv[3]; PhVb=datasv[2]; Ac=datasv[5]; PhVc=datasv[4];
    if(aa.size()==queuelength)aa.pop_back();if(ab.size()==queuelength)ab.pop_back();if(ac.size()==queuelength)ac.pop_back();if(va.size()==queuelength)va.pop_back();if(vb.size()==queuelength)vb.pop_back();if(vc.size()==queuelength)vc.pop_back();
    aa.push_front(Aa); ab.push_front(Ab); ac.push_front(Ac); va.push_front(PhVa);vb.push_front(PhVb); vc.push_front(PhVc);
    // accumulate(aa.front(),aa.back(),0);
    float aasum=0,absum=0,acsum=0,vasum=0,vbsum=0,vcsum=0, Pa=0, Pb=0, Pc=0, P,Sa, Sb, Sc, Qa, Qb, Qc, Q, phi;
    for(int i=0; i<aa.size();i++){aasum=aasum+pow(aa[i],2);  vasum=vasum+pow(va[i],2); Pa=Pa+aa[i]*va[i];}   aasum=sqrt(aasum/(aa.size()+0.000));vasum=sqrt(vasum/(va.size()+0.000)); Pa=Pa/va.size(); VA=vasum;IA=aasum;
    for(int i=0; i<ab.size();i++){absum=absum+pow(ab[i],2);  vbsum=vbsum+pow(vb[i],2); Pb=Pb+ab[i]*vb[i];}   absum=sqrt(absum/(ab.size()+0.000));vbsum=sqrt(vbsum/(vb.size()+0.000)); Pb=Pb/vb.size();VB=vbsum;IB=absum;
    for(int i=0; i<ac.size();i++){acsum=acsum+pow(ac[i],2);  vcsum=vcsum+pow(vc[i],2); Pc=Pc+ac[i]*vc[i];}   acsum=sqrt(acsum/(ac.size()+0.000)); vcsum=sqrt(vcsum/(vc.size()+0.000)); Pc=Pc/vc.size();VC=vcsum;IC=acsum;
    // for(int i=0; i<va.size();i++){vasum=vasum+pow(va[i],2); Pa=Pa+aa[i]*va[i];}     vasum=sqrt(vasum/(va.size()+0.000)); Pa=Pa/va.size();
    //  for(int i=0; i<vb.size();i++){vbsum=vbsum+pow(vb[i],2); Pb=Pb+ab[i]*vb[i];}     vbsum=sqrt(vbsum/(vb.size()+0.000)); Pb=Pb/vb.size();
    //   for(int i=0; i<vc.size();i++){vcsum=vcsum+pow(vc[i],2); Pc=Pc+ac[i]*vc[i];}     vcsum=sqrt(vcsum/(vc.size()+0.000)); Pc=Pc/vc.size();
    P=Pa+Pb+Pc;
    //   P=sqrt(Pa*Pa+Pb*Pb+Pc*Pc);
    Sa=VA*IA; Sb=VB*IB; Sc=VC*IC;
    Qa=sqrt(fabs(Sa*Sa-Pa*Pa)); Qb=sqrt(fabs(Sb*Sb-Pb*Pb)); Qc=sqrt(fabs(Sc*Sc-Pc*Pc)); //Q=sqrt(Qa*Qa+Qb*Qb+Qc*Qc);//
    //   cout<<Pa<<"\t"<<Pb<<"\t"<<Pc<<endl;
    //   cout<<Qa<<"\t"<<Qb<<"\t"<<Qc<<endl; cout<<Sa<<"\t"<<Sb<<"\t"<<Sc<<endl;
    Q=Qa+Qb+Qc;
    phi=(atan(Q/P) /pi) *180;
    //   cout<<"P= "<<P<<"\tQ= "<<Q<<"\tphi= "<<phi<<endl;
    //   printf("%f\t%f\t%f\t\t\t%f\t%f\t%f\t\t%f\n\n",VA,VB,VC,IA,IB,IC,datasv[6]);
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsA_instCVal_mag_f) !=aasum) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsA_instCVal_mag_f,aasum);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsA_t, Hal_getTimeInMs());
//    }
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsB_instCVal_mag_f) !=absum) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsB_instCVal_mag_f,absum);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsB_t, Hal_getTimeInMs());
//    }
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsC_instCVal_mag_f) !=acsum) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsC_instCVal_mag_f,acsum);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsC_t, Hal_getTimeInMs());
//    }//**********************************************
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsA_instCVal_mag_f) !=vasum) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsA_instCVal_mag_f,vasum);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsA_t, Hal_getTimeInMs());
//    }
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsB_instCVal_mag_f) !=vbsum) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsB_instCVal_mag_f,vbsum);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsB_t, Hal_getTimeInMs());
//    }
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsC_instCVal_mag_f) !=  vcsum ) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsC_instCVal_mag_f,  vcsum);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsC_t, Hal_getTimeInMs());
//    }//*************************************************************************************
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotW_instMag_f) != P) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotW_instMag_f, P);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotW_t, Hal_getTimeInMs());
//    }
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotVAr_instMag_f) != Q) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotVAr_instMag_f,Q);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotVAr_t, Hal_getTimeInMs());
//    }
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_instMag_f) != datasv[6]) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_instMag_f, datasv[6]);IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_t, Hal_getTimeInMs());
//    }
//    if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotPF_instMag_f) != cos(phi)) {
    IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotPF_instMag_f, cos(phi));IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotPF_t, Hal_getTimeInMs());
//    }
#endif
    return 0;
}

void* subscribeGOOSEfromrealIED(void *arg)//subscribeGOOSEfromrealIEDstatus
{// add monitoring other IEDs 190716
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    char * interface=para.interface;

    GooseReceiver receiver = GooseReceiver_create();GooseReceiver receiver2 = GooseReceiver_create();GooseReceiver receiver3= GooseReceiver_create();

    printf("Goose receiver  : Using interface %s \n", interface);
    GooseReceiver_setInterfaceId(receiver,interface);GooseReceiver_setInterfaceId(receiver2, interface);GooseReceiver_setInterfaceId(receiver3, interface);

    GooseSubscriber subscriber = GooseSubscriber_create("LIED20CTRL/LLN0$GO$Status", NULL);
    GooseSubscriber subscriber2 = GooseSubscriber_create("LIED20PROT/LLN0$GO$Alarm", NULL);
    GooseSubscriber subscriber3 = GooseSubscriber_create("LIED20MEAS/LLN0$GO$Meas", NULL);

    uint16_t appid1=901, appid2=902, appid3=903;
    GooseSubscriber_setAppId(subscriber, appid1); GooseSubscriber_setAppId(subscriber2, appid2); GooseSubscriber_setAppId(subscriber3, appid3);
    GooseSubscriber_setListener(subscriber, gooseListener, (void*)(&appid1));GooseSubscriber_setListener(subscriber2, gooseListener, (void*)(&appid2));GooseSubscriber_setListener(subscriber3, gooseListener, (void*)(&appid3));
    GooseReceiver_addSubscriber(receiver, subscriber);GooseReceiver_addSubscriber(receiver2, subscriber2);GooseReceiver_addSubscriber(receiver3, subscriber3);
    GooseReceiver_start(receiver);GooseReceiver_start(receiver2);GooseReceiver_start(receiver3);
    signal(SIGINT, sigint_handler);
    while (running) {
    //    Thread_sleep(3);
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
          //      cout<<gostatus.XCBR_Pos<< gostatus.XSWI_Pos<<gostatus.XSWI_Pos2<< gostatus.PTRC_EEHealth<<boolalpha<<gostatus.XCBR_Loc<<endl;
            }
            catch (...)
            {
                printf("*********************************\n");
                continue;
            }
        }
        if (mmsvaluealarm!=NULL) {
            try {
                char buffer[1024];
                MmsValue_printToBuffer(mmsvaluealarm, buffer, 1024);
                string buffers = buffer;
                //      cout<<"string: "<<buffers<<endl;
                buffers = buffers.substr(1, buffers.length() - 2);
                memset(buffer, '\0', sizeof(buffer));
                strcpy(buffer, buffers.c_str());
                //       printf("buffer: %s\n",buffer);
                char *delim = ",";
                char *p;
                goalarm.PIOC_Op_general = strcmp(strtok(buffer, delim), "true") == 0;
                goalarm.XCBR_EEHealth = atoi(strtok(NULL, delim));
                goalarm.LPHD_PwrSupAlm = strcmp(strtok(NULL, delim), "true") == 0;
                goalarm.PSCH_ProTx = strcmp(strtok(NULL, delim), "true") == 0;
                goalarm.PSCH_ProRx = strcmp(strtok(NULL, delim), "true") == 0;
          //          cout<<boolalpha<<goalarm.PIOC_Op_general<<noboolalpha<< goalarm.XCBR_EEHealth<<boolalpha<< goalarm.LPHD_PwrSupAlm<< goalarm.PSCH_ProTx<<goalarm.PSCH_ProRx<<endl;
            }
            catch (...) {
                printf("111122222222222222222222112\n");
                continue;
            }
        }
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
      //            cout<<gomeas.aphsa<<gomeas.aphsb<< gomeas.aphsc<<gomeas.phva<< gomeas.phvb<< gomeas.phvc<<gomeas.totw<<gomeas.totvar<<gomeas.hz<< gomeas.totpf<<endl;
            }
            catch (...)
            {
                printf("3333333333333333333333333333333333\n");
                continue;
            }
        }

    }
    GooseReceiver_stop(receiver);GooseReceiver_stop(receiver2);GooseReceiver_stop(receiver3);
    GooseReceiver_destroy(receiver);GooseReceiver_destroy(receiver2);GooseReceiver_destroy(receiver3);

}
/*
void* subscribeGOOSEfromrealIEDalarm(void *arg){
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    char * interface=para.interface;
    GooseReceiver receiver2 = GooseReceiver_create();//GooseReceiver receiver3= GooseReceiver_create();
    printf("Goose receiver 902: Using interface %s\n",interface);
    GooseReceiver_setInterfaceId(receiver2, interface);//GooseReceiver_setInterfaceId(receiver3, "enp0s8");
    GooseSubscriber subscriber2 = GooseSubscriber_create("LIED20PROT/LLN0$GO$Alarm", NULL);
    // GooseSubscriber subscriber3 = GooseSubscriber_create("LIED20MEAS/LLN0$GO$Meas", NULL);
    uint16_t appid=902;uint16_t *appidp;appidp=&appid;
    GooseSubscriber_setAppId(subscriber2, appid); //GooseSubscriber_setAppId(subscriber3, 903);
    GooseSubscriber_setListener(subscriber2, gooseListener, (void*)appidp);//GooseSubscriber_setListener(subscriber3, gooseListener, NULL);
    GooseReceiver_addSubscriber(receiver2, subscriber2);//GooseReceiver_addSubscriber(receiver3, subscriber);
    GooseReceiver_start(receiver2);//GooseReceiver_start(receiver3);
    signal(SIGINT, sigint_handler);
    while (running) {
        Thread_sleep(300);
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

};*/
/*
void* subscribeGOOSEfromrealIEDmeas(void *arg){
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    char * interface=para.interface;
    GooseReceiver receiver3= GooseReceiver_create();
    printf("Goose receiver 903: Using interface %s\n", interface);
    GooseReceiver_setInterfaceId(receiver3, interface);
    GooseSubscriber subscriber3 = GooseSubscriber_create("LIED20MEAS/LLN0$GO$Meas", NULL);
    uint16_t appid=903;uint16_t *appidp;appidp=&appid;
    GooseSubscriber_setAppId(subscriber3, appid);
    GooseSubscriber_setListener(subscriber3, gooseListener, (void*)appidp);
    GooseReceiver_addSubscriber(receiver3, subscriber3);
    GooseReceiver_start(receiver3);
    signal(SIGINT, sigint_handler);
    while (running) {
        Thread_sleep(300);
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

};*/

/* Callback handler for received SV messages */
static void svUpdateListener (SVSubscriber subscriber, void* parameter, SVSubscriber_ASDU asdu)
{
    uint16_t appid=*(uint16_t*)parameter;
   // printf("svUpdateListener called\n");
    const char* svID = SVSubscriber_ASDU_getSvId(asdu);
/*    if (svID != NULL)
        printf("  svID=(%s)\n", svID);
    printf("  smpCnt: %i\n", SVSubscriber_ASDU_getSmpCnt(asdu));
    printf("  confRev: %u\n", SVSubscriber_ASDU_getConfRev(asdu));
    printf("  appID: %u\n", appid);*/
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
    if (SVSubscriber_ASDU_getDataSize(asdu) >= 4) {//8
#define PRINT_SV
#ifndef PRINT_SV
       if(appid==0x4001)     { datasv[0]=SVSubscriber_ASDU_getFLOAT32(asdu, 0); datasv[1]=SVSubscriber_ASDU_getFLOAT32(asdu, 4);}
       else if(appid==0x4002){  datasv[2]=SVSubscriber_ASDU_getFLOAT32(asdu, 0);datasv[3]=SVSubscriber_ASDU_getFLOAT32(asdu, 4);}
       else if(appid==0x4003){  datasv[4]=SVSubscriber_ASDU_getFLOAT32(asdu, 0); datasv[5]=SVSubscriber_ASDU_getFLOAT32(asdu, 4);}
       else if(appid==5001)  { datasv[6]=SVSubscriber_ASDU_getFLOAT32(asdu, 0);}
#else
        if(appid==0x4001)     { printf(" 0x4001  DATA[0]: %f\n", datasv[0]=SVSubscriber_ASDU_getFLOAT32(asdu, 0));printf("   DATA[1]: %f\n", datasv[1]=SVSubscriber_ASDU_getFLOAT32(asdu, 4));}
        else if(appid==0x4002){ printf(" 0x4002  DATA[0]: %f\n", datasv[2]=SVSubscriber_ASDU_getFLOAT32(asdu, 0));printf("   DATA[1]: %f\n", datasv[3]=SVSubscriber_ASDU_getFLOAT32(asdu, 4));}
        else if(appid==0x4003){ printf(" 0x4003  DATA[0]: %f\n", datasv[4]=SVSubscriber_ASDU_getFLOAT32(asdu, 0));printf("   DATA[1]: %f\n", datasv[5]=SVSubscriber_ASDU_getFLOAT32(asdu, 4));}
      //  else if(appid==5001)  { printf(" 5001  DATA[0]: %f\n",datasv[6]=SVSubscriber_ASDU_getFLOAT32(asdu, 0));}
        else if(appid==5001)  { datasv[6]=SVSubscriber_ASDU_getFLOAT32(asdu, 0);}
#endif
    }
}

void* subscribeSV(void *arg)
{
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    char * interface=para.interface;
    SVReceiver receiver1 = SVReceiver_create(); SVReceiver receiver2 = SVReceiver_create(); SVReceiver receiver3 = SVReceiver_create();SVReceiver receiverhz = SVReceiver_create();
  /*  if (argc > 1) {
        SVReceiver_setInterfaceId(receiver, argv[1]);
        printf("Set interface id: %s\n", argv[1]);
    }*/
  //  else {
        printf("SV subscriber: Using interface %s\n", interface);
        SVReceiver_setInterfaceId(receiver1, interface);SVReceiver_setInterfaceId(receiver2, interface);SVReceiver_setInterfaceId(receiver3, interface);SVReceiver_setInterfaceId(receiverhz, interface);
  //  }
    /* Create a subscriber listening to SV messages with APPID 4000h */
    uint16_t appid1=0x4001;uint16_t appid2=0x4002;uint16_t appid3=0x4003; uint16_t appidhz=5001;
    SVSubscriber subscriber1 = SVSubscriber_create(NULL, appid1);SVSubscriber subscriber2 = SVSubscriber_create(NULL, appid2);SVSubscriber subscriber3 = SVSubscriber_create(NULL, appid3);SVSubscriber subscriberhz = SVSubscriber_create(NULL, appidhz);
    /* Install a callback handler for the subscriber */
    SVSubscriber_setListener(subscriber1, svUpdateListener, (void*)&appid1); SVSubscriber_setListener(subscriber2, svUpdateListener, (void*)&appid2); SVSubscriber_setListener(subscriber3, svUpdateListener, (void*)&appid3);SVSubscriber_setListener(subscriberhz, svUpdateListener, (void*)&appidhz);
    /* Connect the subscriber to the receiver */
    SVReceiver_addSubscriber(receiver1, subscriber1);SVReceiver_addSubscriber(receiver2, subscriber2);SVReceiver_addSubscriber(receiver3, subscriber3);SVReceiver_addSubscriber(receiverhz, subscriberhz);
    /* Start listening to SV messages - starts a new receiver background thread */
    SVReceiver_start(receiver1);  SVReceiver_start(receiver2);  SVReceiver_start(receiver3); SVReceiver_start(receiverhz);
    signal(SIGINT, sigint_handler);
    while (running)
        Thread_sleep(300);
    /* Stop listening to SV messages */
    SVReceiver_stop(receiver1);SVReceiver_stop(receiver2);SVReceiver_stop(receiver3);SVReceiver_stop(receiverhz);
    /* Cleanup and free resources */
    SVReceiver_destroy(receiver1);SVReceiver_destroy(receiver2);SVReceiver_destroy(receiver3);SVReceiver_destroy(receiverhz);
}

/*
void controlHandlerForBinaryOutput(void* parameter, MmsValue* value)  //not used
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
//}

/*void logic_inside(void* arg){
    pass;
}*/

int give_alarm_overcurrent(void* arg){
   // float thresholdA=10*1.4143;
    //alarm PIOC instantaneous overcurrent
  //  if( IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsA_instCVal_mag_f) >thresholdA ||IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsB_cVal_mag_f)>thresholdA|| IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsC_instCVal_mag_f) >thresholdA ||datasv[1]>thresholdA||datasv[3]>thresholdA|| datasv[5] > thresholdA){
    if (IA>thresholdA || IB >thresholdA || IC > thresholdA){
        IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_general,true);
        IedServer_updateQuality(iedServer, IEDMODEL_PROT_PIOC_Op_q,0b1010000000000);//reserved: overflow
        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_t,Hal_getTimeInMs() );
        IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal,0);//XCBR circuit breaker 0 open 1 closed
    //    IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XSWI_Pos_stVal,1);//XSWI circuit switch
        cout<<"***\tOVER CURRENT\t******\tOVER CURRENT\t******\tOVER CURRENT\t******\tOVER CURRENT\t******\tOVER CURRENT\t******\tOVER CURRENT\t******\tOVER CURRENT\t***"<<endl;
        return 1;
    }
#define close_breaker_by_hand 1
#if close_breaker_by_hand == 0
    else{
        IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_general,false);
        if(IedServer_getInt32AttributeValue(iedServer,IEDMODEL_PROT_PIOC_Op_q)!=0)IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_t,Hal_getTimeInMs() );
        IedServer_updateQuality(iedServer, IEDMODEL_PROT_PIOC_Op_q,0);
        if( IedServer_getInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal)==0)IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal,1);//XCBR circuit breaker
    //    if( IedServer_getInt32AttributeValue(iedServer, IEDMODEL_CTRL_XSWI_Pos_stVal)!=0)IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XSWI_Pos_stVal,0);//XSWI circuit switch
    }
#endif
  /*
    IEDMODEL_CTRL_XCBR_Pos_stVal                                        O
    IEDMODEL_CTRL_XSWI_Pos_stVal                                        O
    IEDMODEL_PROT_PIOC_Op_general(general, q,t)                         O
    ---
    IEDMODEL_PROT_LPHD_PwrSupAlm_stVal                                  X LPHD physical device informaiton, external power supply alarm
    IEDMODEL_PROT_PSCH_ProRx_stVal, IEDMODEL_PROT_PSCH_ProTx_stVal      X protection scheme, teleprotection signal received/transmitted
    IEDMODEL_PROT_XCBR_EEHealth_stVal                                   X circuit breaker, eehealth: external equipment health
    */
  return 0;
}

void * copy_GOOSEfrom_real_IED(){  // NOT used
#define update_GOOSE_via_realIED 0
#if     update_GOOSE_via_realIED ==1
copy_GOOSEfrom_real_IED_begin:
    /*UPDATE STATUS*/
    if(stnum901!=stnum901last) {
        if (IedServer_getInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal) != gostatus.XCBR_Pos) {
            IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Pos_stVal, gostatus.XCBR_Pos);
        }
        if (IedServer_getInt32AttributeValue(iedServer, IEDMODEL_CTRL_XSWI_Pos_stVal) != gostatus.XSWI_Pos) {
            IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_XSWI_Pos_stVal, gostatus.XSWI_Pos);
        }
        if (IedServer_getInt32AttributeValue(iedServer, IEDMODEL_CTRL_PTRC_EEHealth_stVal) !=gostatus.PTRC_EEHealth) {
            IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_CTRL_PTRC_EEHealth_stVal,gostatus.PTRC_EEHealth);
        }
        if (IedServer_getBooleanAttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Loc_stVal) != gostatus.XCBR_Loc) {
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_CTRL_XCBR_Loc_stVal, gostatus.XCBR_Loc);
        }
        stnum901last=stnum901;
        cout<<"status updates 901\t"<<stnum901last<<"\t"<<stnum901<<endl;
    }
    /*UPDATE ALARM*/
    if(stnum902!=stnum902last) {
        if (IedServer_getBooleanAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_general) !=goalarm.PIOC_Op_general) {
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_general,goalarm.PIOC_Op_general);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_PROT_PIOC_Op_t, Hal_getTimeInMs());
        }
        if (IedServer_getInt32AttributeValue(iedServer, IEDMODEL_PROT_XCBR_EEHealth_stVal) !=goalarm.XCBR_EEHealth) {
            IedServer_updateInt32AttributeValue(iedServer, IEDMODEL_PROT_XCBR_EEHealth_stVal,goalarm.XCBR_EEHealth);
        }
        if (IedServer_getBooleanAttributeValue(iedServer, IEDMODEL_PROT_LPHD_PwrSupAlm_stVal) !=goalarm.LPHD_PwrSupAlm) {
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_LPHD_PwrSupAlm_stVal,goalarm.LPHD_PwrSupAlm);
        }
        if (IedServer_getBooleanAttributeValue(iedServer, IEDMODEL_PROT_PSCH_ProTx_stVal) != goalarm.PSCH_ProTx) {
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PSCH_ProTx_stVal, goalarm.PSCH_ProTx);
        }
        if (IedServer_getBooleanAttributeValue(iedServer, IEDMODEL_PROT_PSCH_ProRx_stVal) != goalarm.PSCH_ProRx) {
            IedServer_updateBooleanAttributeValue(iedServer, IEDMODEL_PROT_PSCH_ProRx_stVal, goalarm.PSCH_ProRx);
        }
        stnum902last=stnum902;
        cout<<"alarm updates 902\t"<<stnum902last<<"\t"<<stnum902<<endl;
    }
    /*UPDATE MEASurement*/
    if(stnum903!=stnum903last) {
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsA_instCVal_mag_f) !=gomeas.MMXU_A_phsA_instCVal_mag_f__) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsA_instCVal_mag_f,gomeas.MMXU_A_phsA_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsA_t, Hal_getTimeInMs());
        }
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsB_instCVal_mag_f) !=gomeas.MMXU_A_phsB_instCVal_mag_f__) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsB_instCVal_mag_f,gomeas.MMXU_A_phsB_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsB_t, Hal_getTimeInMs());
        }
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsC_instCVal_mag_f) !=gomeas.MMXU_A_phsC_instCVal_mag_f__) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsC_instCVal_mag_f,gomeas.MMXU_A_phsC_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_A_phsC_t, Hal_getTimeInMs());
        }//**********************************************
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsA_instCVal_mag_f) !=gomeas.MMXU_PhV_phsA_instCVal_mag_f__) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsA_instCVal_mag_f,gomeas.MMXU_PhV_phsA_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsA_t, Hal_getTimeInMs());
        }
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsB_instCVal_mag_f) !=gomeas.MMXU_PhV_phsB_instCVal_mag_f__) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsB_instCVal_mag_f,gomeas.MMXU_PhV_phsB_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsB_t, Hal_getTimeInMs());
        }
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsC_instCVal_mag_f) !=gomeas.MMXU_PhV_phsC_instCVal_mag_f__) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsC_instCVal_mag_f,gomeas.MMXU_PhV_phsC_instCVal_mag_f__);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_PhV_phsC_t, Hal_getTimeInMs());
        }//**********************************************
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotW_instMag_f) != gomeas.totw) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotW_instMag_f, gomeas.totw);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotW_t, Hal_getTimeInMs());
        }
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotVAr_instMag_f) != gomeas.totvar) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotVAr_instMag_f, gomeas.totvar);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotVAr_t, Hal_getTimeInMs());
        }
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_instMag_f) != gomeas.hz) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_instMag_f, gomeas.hz);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_Hz_t, Hal_getTimeInMs());
        }
        if (IedServer_getFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotPF_instMag_f) != gomeas.totpf) {
            IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotPF_instMag_f, gomeas.totpf);
            IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_MEAS_MMXU_TotPF_t, Hal_getTimeInMs());
        }
        stnum903last=stnum903;
        cout<<"measurement updates 903\t"<<stnum903last<<"\t"<<stnum903<<endl;
    }/*UPDATE FINISHED*/
    if(running)goto copy_GOOSEfrom_real_IED_begin;
#endif
}



void* goosepublisherMAIN(void *arg){
    struct fun_para_g para;
    para=*(struct fun_para_g *)arg;
    iedServer = IedServer_create(&iedModel);
//	if (argc > 1) {
    char* ethernetIfcID = para.interface;
    int portnumber=para.portnumber;
 //   ethernetIfcID="enp0s8";
    printf("GOOSE publisher: Using GOOSE interface: %s , port: %d \n", ethernetIfcID,portnumber);
    IedServer_setGooseInterfaceId(iedServer, ethernetIfcID);
    //}
    /* MMS server will be instructed to start listening to client connections. */
    IedServer_start(iedServer, portnumber);
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
//    float anIn1=0.1f;
    while (running) {
        IedServer_lockDataModel(iedServer);

  //      monitor_other_IEDs_for_status(nullptr);
        update_MEAS_from_SV(nullptr);

        IedServer_unlockDataModel(iedServer);
        Thread_sleep(300);
    }
    /* stop MMS server - close TCP server socket and all client sockets */
    IedServer_stop(iedServer);
    /* Cleanup - free all resources */
    IedServer_destroy(iedServer);
}

void* run1(void* arg){
//    monitor_other_IEDs_for_status(nullptr);
//    update_MEAS_from_SV(nullptr);
    while(running)
        if (busbar_protection(nullptr) == 1)
            while (1) {
                Thread_sleep(2000);
                if (breaker_failure_protection(nullptr) == 1)
                    while (1) {
                        Thread_sleep(500);
                        if (check_status_for_XCBR_closed(nullptr) == 1)
                            break;
                    }
                if (check_status_for_XCBR_closed(nullptr) == 1)break;
            }
}
/*
void* run2(void* arg){
    while(running)
        if (breaker_failure_protection(nullptr) == 1)while (1) { if (check_status_for_XCBR_closed(nullptr) == 1)break; }
}*/

void* run3(void* arg){  //not used now
    while(running)
        if (underfrequency_load_shedding(nullptr) == 1)while (1) { if (check_status_for_XCBR_closed(nullptr) == 1)break; }
}
#include "iec61850_common.h"
#include "stack_config.h"
#include "goose_publisher.h"
PhyComAddress addr1;//={CONFIG_GOOSE_DEFAULT_PRIORITY,CONFIG_GOOSE_DEFAULT_VLAN_ID,1001+ (int)parag[1].portnumber, {0x01, 0x0c, 0xcd, 0x01, 0x00, 0x00}};
PhyComAddress addr2;//={CONFIG_GOOSE_DEFAULT_PRIORITY,CONFIG_GOOSE_DEFAULT_VLAN_ID,1002+parag[0].portnumber, {0x01, 0x0c, 0xcd, 0x01, 0x00, 0x00}};
PhyComAddress addr3;//={CONFIG_GOOSE_DEFAULT_PRIORITY,CONFIG_GOOSE_DEFAULT_VLAN_ID,1003+parag[0].portnumber, {0x01, 0x0c, 0xcd, 0x01, 0x00, 0x00}};
fun_para_g parag[10];
int main(int argc, char** argv) {
    mmsvaluestatus=NULL; mmsvaluealarm=NULL;mmsvaluemeas=NULL;
    running = 1;
    pthread_t thread[10];
    int t_id[10];
    //struct fun_para_g parag[10];
    if (argc > 1) {
        for (int i = 0; i < 10; i++){
            parag[i].interface = argv[1];
            parag[i].portnumber=atoi(argv[2]);
        }
    }
    else {
        for (int i = 0; i < 10; i++) {
            parag[i].interface = "enp0s8";
            parag[i].portnumber = 5001;
        }
    }
    uint16_t appidadd=(uint16_t) ( parag[0].portnumber%5000+UINT_MAX+1);
    addr1={CONFIG_GOOSE_DEFAULT_PRIORITY,CONFIG_GOOSE_DEFAULT_VLAN_ID,901+appidadd*3, {0x01, 0x0c, 0xcd, 0x01, 0x00, 0x00}};
    addr2={CONFIG_GOOSE_DEFAULT_PRIORITY,CONFIG_GOOSE_DEFAULT_VLAN_ID,902+appidadd*3, {0x01, 0x0c, 0xcd, 0x01, 0x00, 0x00}};
    addr3={CONFIG_GOOSE_DEFAULT_PRIORITY,CONFIG_GOOSE_DEFAULT_VLAN_ID,903+appidadd*3, {0x01, 0x0c, 0xcd, 0x01, 0x00, 0x00}};
    t_id [ 0 ] = pthread_create(&thread[0], NULL , goosepublisherMAIN , & parag [ 0 ] ) ;
    t_id [ 1 ] = pthread_create(&thread[1], NULL , subscribeGOOSEfromrealIED , & parag [ 1 ] ) ;
    t_id [ 2 ] = pthread_create(&thread[2], NULL , subscribeSV , & parag [ 2 ] ) ;
    t_id [ 3 ] = pthread_create(&thread[3], NULL , run1 , & parag [ 3 ] ) ;
 //   t_id [ 4 ] = pthread_create(&thread[4], NULL , run2 , & parag [ 4 ] ) ;
   // t_id [ 5 ] = pthread_create(&thread[5], NULL , run3 ,  & parag [ 5 ] ) ;

    pthread_join(thread[0], NULL ) ;
    pthread_join(thread[1], NULL ) ;
    pthread_join(thread[2], NULL ) ;
    pthread_join(thread[3], NULL ) ;
//    pthread_join(thread[4], NULL ) ;
  //  pthread_join(thread[5], NULL ) ;
//  subscribeGOOSEfromrealIED(&parag[1]);


    return 0;
} /* main() */

