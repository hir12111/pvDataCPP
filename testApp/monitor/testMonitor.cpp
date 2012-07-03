/* testPVdata.cpp */
/**
 * Copyright - See the COPYRIGHT that is included with this distribution.
 * EPICS pvDataCPP is distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution.
 */
/* Author:  Marty Kraimer Date: 2010.11 */

#include <cstddef>
#include <cstdlib>
#include <cstddef>
#include <string>
#include <cstring>
#include <cstdio>

#include <epicsAssert.h>
#include <epicsExit.h>

#include <pv/requester.h>
#include <pv/pvIntrospect.h>
#include <pv/pvData.h>
#include <pv/monitor.h>
#include <pv/standardField.h>


using namespace epics::pvData;

static String allProperties("alarm,timeStamp,display,control,valueAlarm");
static StandardFieldPtr standardField = getStandardField();

class MonitorImpl : public Monitor
{
public:
    POINTER_DEFINITIONS(MonitorImpl);
    MonitorImpl() {}
    virtual ~MonitorImpl() {}
    virtual void destroy() {}
    virtual Status start()
    {
        printf("start called\n");
        return status;
    }
    virtual Status stop()
    {
        printf("stop called\n");
        return status;
    }
    virtual MonitorElementPtr poll()
    {
        printf("poll called\n");
        return emptyElement;
    }
    virtual void release(MonitorElementPtr & monitorElement)
    {
        printf("release called\n");
    }
private:
    Status status;
    MonitorElementPtr emptyElement;
};

static void testMonitor()
{
    MonitorPtr monitor(new MonitorImpl());
    monitor->start();
    MonitorElementPtr monitorElement = monitor->poll();
    monitor->release(monitorElement);
    monitor->stop();
}


int main(int argc,char *argv[])
{
    testMonitor();
    return(0);
}

