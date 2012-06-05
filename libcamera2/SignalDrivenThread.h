/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*!
 * \file      SignalDrivenThread.h
 * \brief     header file for general thread ( for camera hal2 implementation )
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/05/31
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 */
 


#ifndef SIGNAL_DRIVEN_THREAD_H
#define SIGNAL_DRIVEN_THREAD_H

#include <utils/threads.h>

namespace android {

#define SIGNAL_THREAD_TERMINATE     (1<<0)
#define SIGNAL_THREAD_PAUSE         (1<<1)

#define SIGNAL_THREAD_COMMON_LAST   (1<<3)

class SignalDrivenThread : public Thread {
public:
                        SignalDrivenThread();
                        SignalDrivenThread(const char* name, 
                            int32_t priority, size_t stack);
    virtual             ~SignalDrivenThread();

            status_t    SetSignal(uint32_t signal);
            

            uint32_t    GetProcessingSignal();
            //void        ClearProcessingSignal(uint32_t signal);
               

private:
            status_t    readyToRun();
    virtual status_t    readyToRunInternal() = 0;
    
            bool        threadLoop();
    virtual void        threadLoopInternal() = 0;

            void        ClearSignal();
    
            uint32_t    m_receivedSignal;
            uint32_t    m_processingSignal;  

            Mutex       m_signalMutex;
            Condition   m_threadCondition;
};

}; // namespace android

#endif

