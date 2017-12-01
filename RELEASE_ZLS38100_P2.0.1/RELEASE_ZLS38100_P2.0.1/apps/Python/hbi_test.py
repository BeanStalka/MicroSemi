#!/usr/bin/python
#add lib search directory path into sys.path
import sys
import threading
sys.path.append('/home/pi/ZLS38100_P2.0.0/RELEASE_ZLS38100_P2.0.0/libs')
import sr
sr.quit=0

def initSpeechRecognitionModule():
    status=sr.init()
    if status < 0 :
        print 'Error in SR init'
        return status
    while sr.quit==0:
        command=sr.getCommand();
        print "Trigger Received 0x%x\n" %(command)
    status = sr.term()
    print "Driver Terminated"
    return status

if (__name__ == "__main__"):
    player_id=1
    params="\"to\":\"next\",\"playerid\":%d" %(player_id)
    print params
    #initSpeechRecognitionModule()