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
 * \file      ExynosCameraHWInterface2.cpp
 * \brief     source file for Android Camera API 2.0 HAL
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/07/10
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 *
 * - 2012/07/10 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   2nd Release
 *
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraHWInterface2"
#include <utils/Log.h>

#include "ExynosCameraHWInterface2.h"
#include "exynos_format.h"



namespace android {


// temporarily copied from EmulatedFakeCamera2
// TODO : implement our own codes
status_t constructDefaultRequestInternal(
        int request_template,
        camera_metadata_t **request,
        bool sizeRequest);

status_t constructStaticInfo(
        camera_metadata_t **info,
        bool sizeRequest);

int get_pixel_depth(uint32_t fmt)
{
    int depth = 0;

    switch (fmt) {
    case V4L2_PIX_FMT_JPEG:
        depth = 8;
        break;

    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420M:
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV12MT:
        depth = 12;
        break;

    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SBGGR16:
        depth = 16;
        break;

    case V4L2_PIX_FMT_RGB32:
        depth = 32;
        break;
    default:
        ALOGE("Get depth failed(format : %d)", fmt);
        break;
    }

    return depth;
}

int cam_int_s_fmt(node_info_t *node)
{
    struct v4l2_format v4l2_fmt;
    unsigned int framesize;
    int ret;

    memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));

    v4l2_fmt.type = node->type;
    framesize = (node->width * node->height * get_pixel_depth(node->format)) / 8;

    if (node->planes >= 1) {
        v4l2_fmt.fmt.pix_mp.width       = node->width;
        v4l2_fmt.fmt.pix_mp.height      = node->height;
        v4l2_fmt.fmt.pix_mp.pixelformat = node->format;
        v4l2_fmt.fmt.pix_mp.field       = V4L2_FIELD_ANY;
    } else {
        ALOGE("%s:S_FMT, Out of bound : Number of element plane",__FUNCTION__);
    }

    /* Set up for capture */
    ret = exynos_v4l2_s_fmt(node->fd, &v4l2_fmt);

    if (ret < 0)
        ALOGE("%s: exynos_v4l2_s_fmt fail (%d)",__FUNCTION__, ret);

    return ret;
}

int cam_int_reqbufs(node_info_t *node)
{
    struct v4l2_requestbuffers req;
    int ret;

    req.count = node->buffers;
    req.type = node->type;
    req.memory = node->memory;

    ret = exynos_v4l2_reqbufs(node->fd, &req);

    if (ret < 0)
        ALOGE("%s: VIDIOC_REQBUFS (fd:%d) failed (%d)",__FUNCTION__,node->fd, ret);

    return req.count;
}

int cam_int_qbuf(node_info_t *node, int index)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int i;
    int ret = 0;

    v4l2_buf.m.planes   = planes;
    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.index      = index;
    v4l2_buf.length     = node->planes;

    for(i = 0; i < node->planes; i++){
        v4l2_buf.m.planes[i].m.fd = (int)(node->buffer[index].fd.extFd[i]);
        v4l2_buf.m.planes[i].length  = (unsigned long)(node->buffer[index].size.extS[i]);
    }

    ret = exynos_v4l2_qbuf(node->fd, &v4l2_buf);

    if (ret < 0)
        ALOGE("%s: cam_int_qbuf failed (index:%d)(ret:%d)",__FUNCTION__, index, ret);

    return ret;
}

int cam_int_streamon(node_info_t *node)
{
    enum v4l2_buf_type type = node->type;
    int ret;

    ret = exynos_v4l2_streamon(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMON failed (%d)",__FUNCTION__, ret);

    ALOGV("On streaming I/O... ... fd(%d)", node->fd);

    return ret;
}

int cam_int_streamoff(node_info_t *node)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	int ret;

	ALOGV("Off streaming I/O... fd(%d)", node->fd);
	ret = exynos_v4l2_streamoff(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMOFF failed (%d)",__FUNCTION__, ret);

	return ret;
}

int cam_int_dqbuf(node_info_t *node)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;

    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.m.planes   = planes;
    v4l2_buf.length     = node->planes;

    ret = exynos_v4l2_dqbuf(node->fd, &v4l2_buf);
    if (ret < 0)
        ALOGE("%s: VIDIOC_DQBUF failed (%d)",__FUNCTION__, ret);

    return v4l2_buf.index;
}

int cam_int_s_input(node_info_t *node, int index)
{
    int ret;

    ret = exynos_v4l2_s_input(node->fd, index);
    if (ret < 0)
        ALOGE("%s: VIDIOC_S_INPUT failed (%d)",__FUNCTION__, ret);

    return ret;
}


gralloc_module_t const* ExynosCameraHWInterface2::m_grallocHal;

RequestManager::RequestManager(SignalDrivenThread* main_thread):
    m_numOfEntries(0),
    m_entryInsertionIndex(0),
    m_entryProcessingIndex(0),
    m_entryFrameOutputIndex(0)
{
    m_metadataConverter = new MetadataConverter;
    m_mainThread = main_thread;
    for (int i=0 ; i<NUM_MAX_REQUEST_MGR_ENTRY; i++) {
        //entries[i].status = EMPTY;
        memset(&(entries[i]), 0x00, sizeof(request_manager_entry_t));
        entries[i].internal_shot.ctl.request.frameCount = -1;
    }
    return;
}

RequestManager::~RequestManager()
{
    return;
}

int RequestManager::GetNumEntries()
{
    return m_numOfEntries;
}

bool RequestManager::IsRequestQueueFull()
{
    Mutex::Autolock lock(m_requestMutex);
    if (m_numOfEntries>=NUM_MAX_REQUEST_MGR_ENTRY)
        return true;
    else
        return false;
}

void RequestManager::RegisterRequest(camera_metadata_t * new_request)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);

    Mutex::Autolock lock(m_requestMutex);

    request_manager_entry * newEntry = NULL;
    int newInsertionIndex = ++m_entryInsertionIndex;
    if (newInsertionIndex >= NUM_MAX_REQUEST_MGR_ENTRY)
        newInsertionIndex = 0;
    ALOGV("DEBUG(%s): got lock, new insertIndex(%d), cnt before reg(%d)", __FUNCTION__,newInsertionIndex,m_numOfEntries );


    newEntry = &(entries[newInsertionIndex]);

    if (newEntry->status!=EMPTY) {
        ALOGV("DEBUG(%s): Circular buffer abnormal ", __FUNCTION__);
        return;
    }
    newEntry->status = REGISTERED;
    newEntry->original_request = new_request;
    // TODO : allocate internal_request dynamically
    m_metadataConverter->ToInternalShot(new_request, &(newEntry->internal_shot));
    newEntry->output_stream_count = newEntry->internal_shot.ctl.request.numOutputStream;

    m_numOfEntries++;
    m_entryInsertionIndex = newInsertionIndex;


        Dump();
    ALOGV("## RegisterReq DONE num(%d), insert(%d), processing(%d), frame(%d), (frameCnt(%d))",
     m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex, newEntry->internal_shot.ctl.request.frameCount);
}

void RequestManager::DeregisterRequest(camera_metadata_t ** deregistered_request)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    Mutex::Autolock lock(m_requestMutex);

    request_manager_entry * currentEntry =  &(entries[m_entryFrameOutputIndex]);

    if (currentEntry->status!=PROCESSING) {
        ALOGD("DBG(%s): Circular buffer abnormal. processing(%d), frame(%d), status(%d) ", __FUNCTION__
        , m_entryProcessingIndex, m_entryFrameOutputIndex,(int)(currentEntry->status));
        return;
    }
    if (deregistered_request)  *deregistered_request = currentEntry->original_request;

    currentEntry->status = EMPTY;
    currentEntry->original_request = NULL;
    memset(&(currentEntry->internal_shot), 0, sizeof(camera2_ctl_metadata_NEW_t));
    currentEntry->internal_shot.ctl.request.frameCount = -1;
    currentEntry->output_stream_count = 0;
    currentEntry->dynamic_meta_vaild = false;
    m_numOfEntries--;
    Dump();
    ALOGV("## DeRegistReq DONE num(%d), insert(%d), processing(%d), frame(%d)",
     m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);

    return;
}

bool RequestManager::PrepareFrame(size_t* num_entries, size_t* frame_size,
                camera_metadata_t ** prepared_frame)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    Mutex::Autolock lock(m_requestMutex);
    status_t res = NO_ERROR;
    int tempFrameOutputIndex = m_entryFrameOutputIndex + 1;
    if (tempFrameOutputIndex >= NUM_MAX_REQUEST_MGR_ENTRY)
        tempFrameOutputIndex = 0;
    request_manager_entry * currentEntry =  &(entries[tempFrameOutputIndex]);
    ALOGV("DEBUG(%s): processing(%d), frameOut(%d), insert(%d) recentlycompleted(%d)", __FUNCTION__,
        m_entryProcessingIndex, m_entryFrameOutputIndex, m_entryInsertionIndex, m_completedIndex);

    if (m_completedIndex != tempFrameOutputIndex) {
        ALOGV("DEBUG(%s): frame left behind : completed(%d), preparing(%d)", __FUNCTION__, m_completedIndex,tempFrameOutputIndex);

        request_manager_entry * currentEntry2 =  &(entries[tempFrameOutputIndex]);
        currentEntry2->status = EMPTY;
        currentEntry2->original_request = NULL;
        memset(&(currentEntry2->internal_shot), 0, sizeof(camera2_ctl_metadata_NEW_t));
        currentEntry2->internal_shot.ctl.request.frameCount = -1;
        currentEntry2->output_stream_count = 0;
        currentEntry2->dynamic_meta_vaild = false;
        m_numOfEntries--;
        Dump();
        tempFrameOutputIndex = m_completedIndex;
        currentEntry =  &(entries[tempFrameOutputIndex]);
    }

    if (currentEntry->output_stream_count!=0) {
        ALOGD("DBG(%s): Circular buffer has remaining output : stream_count(%d)", __FUNCTION__, currentEntry->output_stream_count);
        return false;
    }



    if (currentEntry->status!=PROCESSING) {
        ALOGD("DBG(%s): Circular buffer abnormal status(%d)", __FUNCTION__, (int)(currentEntry->status));

        return false;
    }
    m_entryFrameOutputIndex = tempFrameOutputIndex;
    m_tempFrameMetadata = place_camera_metadata(m_tempFrameMetadataBuf, 2000, 10, 500); //estimated
    res = m_metadataConverter->ToDynamicMetadata(&(currentEntry->internal_shot),
                m_tempFrameMetadata);
    if (res!=NO_ERROR) {
        ALOGE("ERROR(%s): ToDynamicMetadata (%d) ", __FUNCTION__, res);
        return false;
    }
    *num_entries = get_camera_metadata_entry_count(m_tempFrameMetadata);
    *frame_size = get_camera_metadata_size(m_tempFrameMetadata);
    *prepared_frame = m_tempFrameMetadata;
    ALOGV("## PrepareFrame DONE: frameOut(%d) frameCnt-req(%d)", m_entryFrameOutputIndex,
        currentEntry->internal_shot.ctl.request.frameCount);
        Dump();
    return true;
}

int RequestManager::MarkProcessingRequest(ExynosBuffer* buf)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    Mutex::Autolock lock(m_requestMutex);
    struct camera2_shot_ext * shot_ext;
    int targetStreamIndex = 0;

    // TODO : in the case of Request underrun, insert a bubble

    if (m_numOfEntries == 0)  {
        ALOGV("DEBUG(%s): Request Manager Empty ", __FUNCTION__);
        return -1;
    }

    if ((m_entryProcessingIndex == m_entryInsertionIndex)
        && (entries[m_entryProcessingIndex].status == PROCESSING)) {
        ALOGV("## MarkProcReq skipping(request underrun) -  num(%d), insert(%d), processing(%d), frame(%d)",
         m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);
        return -1;
    }

    request_manager_entry * newEntry = NULL;
    int newProcessingIndex = m_entryProcessingIndex + 1;
    if (newProcessingIndex >= NUM_MAX_REQUEST_MGR_ENTRY)
        newProcessingIndex = 0;

    newEntry = &(entries[newProcessingIndex]);

    if (newEntry->status!=REGISTERED) {
        ALOGV("DEBUG(%s): Circular buffer abnormal ", __FUNCTION__);
        Dump();
        return -1;
    }
    newEntry->status = PROCESSING;
    // TODO : replace the codes below with a single memcpy of pre-converted 'shot'

    shot_ext = (struct camera2_shot_ext *)(buf->virt.extP[1]);
    memset(shot_ext, 0x00, sizeof(struct camera2_shot_ext));

    shot_ext->request_sensor = 1;
    for (int i = 0; i < newEntry->output_stream_count; i++) {
        // TODO : match with actual stream index;
        targetStreamIndex = newEntry->internal_shot.ctl.request.outputStreams[i];

        if (targetStreamIndex==0) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerP", __FUNCTION__, i);
            shot_ext->request_scp = 1;
        }
        else if (targetStreamIndex==1) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerC", __FUNCTION__, i);
            shot_ext->request_scc = 1;
        }
        else {
            ALOGV("DEBUG(%s): outputstreams(%d) has abnormal value(%d)", __FUNCTION__, i, targetStreamIndex);
        }
    }
	shot_ext->shot.ctl.request.metadataMode = METADATA_MODE_FULL;
	shot_ext->shot.magicNumber = 0x23456789;
	shot_ext->shot.ctl.sensor.exposureTime = 0;
	shot_ext->shot.ctl.sensor.frameDuration = 33*1000*1000;
	shot_ext->shot.ctl.sensor.sensitivity = 0;

    shot_ext->shot.ctl.scaler.cropRegion[0] = 0;
    shot_ext->shot.ctl.scaler.cropRegion[1] = 0;
    shot_ext->shot.ctl.scaler.cropRegion[2] = 1920;

    // HACK : use id field for identifier
    shot_ext->shot.ctl.request.id = newEntry->internal_shot.ctl.request.frameCount;

    //newEntry->request_serial_number = m_request_serial_number;

    //m_request_serial_number++;

    m_entryProcessingIndex = newProcessingIndex;

    Dump();
    ALOGV("## MarkProcReq DONE totalentry(%d), insert(%d), processing(%d), frame(%d) frameCnt(%d)",
    m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex, newEntry->internal_shot.ctl.request.frameCount);

    return m_entryProcessingIndex;
}

void RequestManager::NotifyStreamOutput(int index, int stream_id)
{
    ALOGV("DEBUG(%s): reqIndex(%d), stream_id(%d)", __FUNCTION__, index, stream_id);
    if (index < 0) return;
    entries[index].output_stream_count--;  //TODO : match stream id also
    CheckCompleted(index);

    return;
}

void RequestManager::CheckCompleted(int index)
{
    ALOGV("DEBUG(%s): reqIndex(%d)", __FUNCTION__, index);
    if (entries[index].output_stream_count==0 && entries[index].dynamic_meta_vaild) {
        ALOGV("DEBUG(%s): index[%d] completed and sending SIGNAL_MAIN_STREAM_OUTPUT_DONE", __FUNCTION__, index);
        Dump();
        m_completedIndex = index;
        m_mainThread->SetSignal(SIGNAL_MAIN_STREAM_OUTPUT_DONE);
    }
    return;
}
/*
int RequestManager::FindEntryIndexByRequestSerialNumber(int serial_num)
{
    for (int i=0 ; i<NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        if (entries[i].internal_shot.ctl.request.frameCount == serial_num) {
            if (entries[i].status == PROCESSING) {
                return i;
            }
            else {
                ALOGD("DBG(%s): abnormal entry[%d] status(%d)", __FUNCTION__, i, entries[i].status);

            }
        }
    }
    return -1;
}
*/
void RequestManager::ApplyDynamicMetadata(int index)
{
    ALOGV("DEBUG(%s): reqIndex(%d)", __FUNCTION__, index);
    entries[index].dynamic_meta_vaild = true;

    // TODO : move some code of PrepareFrame here

    CheckCompleted(index);
}

void RequestManager::DumpInfoWithIndex(int index)
{
    camera2_ctl_metadata_NEW_t * currMetadata = &(entries[index].internal_shot);

    ALOGV("####   frameCount(%d) exposureTime(%lld) ISO(%d)",
        currMetadata->ctl.request.frameCount,
        currMetadata->ctl.sensor.exposureTime,
        currMetadata->ctl.sensor.sensitivity);
    if (currMetadata->ctl.request.numOutputStream==0)
        ALOGV("####   No output stream selected");
    else if (currMetadata->ctl.request.numOutputStream==1)
        ALOGV("####   OutputStreamId : %d", currMetadata->ctl.request.outputStreams[0]);
    else if (currMetadata->ctl.request.numOutputStream==2)
        ALOGV("####   OutputStreamId : %d, %d", currMetadata->ctl.request.outputStreams[0],
            currMetadata->ctl.request.outputStreams[1]);
    else
        ALOGV("####   OutputStream num (%d) abnormal ", currMetadata->ctl.request.numOutputStream);
}

void    RequestManager::UpdateOutputStreamInfo(struct camera2_shot_ext *shot_ext, int index)
{
    ALOGV("DEBUG(%s): updating info with reqIndex(%d)", __FUNCTION__, index);
    if (index<0)
        return;
    int targetStreamIndex = 0;
    request_manager_entry * newEntry = &(entries[index]);
    shot_ext->request_sensor = 1;
    shot_ext->request_scc = 0;
    shot_ext->request_scp = 0;
    for (int i = 0; i < newEntry->output_stream_count; i++) {
        // TODO : match with actual stream index;
        targetStreamIndex = newEntry->internal_shot.ctl.request.outputStreams[i];

        if (targetStreamIndex==0) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerP", __FUNCTION__, i);
            shot_ext->request_scp = 1;
        }
        else if (targetStreamIndex==1) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerC", __FUNCTION__, i);
            shot_ext->request_scc = 1;
        }
        else {
            ALOGV("DEBUG(%s): outputstreams(%d) has abnormal value(%d)", __FUNCTION__, i, targetStreamIndex);
        }
    }
}

void    RequestManager::RegisterTimestamp(int index, nsecs_t * frameTime)
{
    ALOGD("DEBUG(%s): updating timestamp for reqIndex(%d) (%lld)", __FUNCTION__, index, *frameTime);
    request_manager_entry * currentEntry = &(entries[index]);
    currentEntry->internal_shot.dm.sensor.timeStamp = *((uint64_t*)frameTime);
    ALOGD("DEBUG(%s): applied timestamp for reqIndex(%d) (%lld)", __FUNCTION__,
        index, currentEntry->internal_shot.dm.sensor.timeStamp);
}

uint64_t  RequestManager::GetTimestamp(int index)
{
    request_manager_entry * currentEntry = &(entries[index]);
    uint64_t frameTime = currentEntry->internal_shot.dm.sensor.timeStamp;
    ALOGD("DEBUG(%s): Returning timestamp for reqIndex(%d) (%lld)", __FUNCTION__, index, frameTime);
    return frameTime;
}


void RequestManager::Dump(void)
{
//    ALOGV("DEBUG(%s): updating timestamp for reqIndex(%d) (%lld)", __FUNCTION__, index, *frameTime);
    int i = 0;
    request_manager_entry * currentEntry;
    ALOGV("## Dump  totalentry(%d), insert(%d), processing(%d), frame(%d)",
    m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);

    for (i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        currentEntry =  &(entries[i]);
        ALOGV("[%2d] status[%d] frameCnt[%3d] numOutput[%d]", i,
        currentEntry->status, currentEntry->internal_shot.ctl.request.frameCount,
            currentEntry->output_stream_count);
    }
}

ExynosCameraHWInterface2::ExynosCameraHWInterface2(int cameraId, camera2_device_t *dev):
            m_requestQueueOps(NULL),
            m_frameQueueOps(NULL),
            m_callbackCookie(NULL),
            m_numOfRemainingReqInSvc(0),
            m_isRequestQueuePending(false),
            m_isRequestQueueNull(true),
            m_isSensorThreadOn(false),
            m_isSensorStarted(false),
            m_ionCameraClient(0),
            m_initFlag1(false),
            m_initFlag2(false),
            m_numExpRemainingOutScp(0),
            m_numExpRemainingOutScc(0),
            m_numBayerQueueList(0),
            m_numBayerDequeueList(0),
            m_numBayerQueueListRemainder(0),
            m_scp_flushing(false),
            m_closing(false),
            m_ispInputIndex(-2),
            m_lastTimeStamp(0),
            m_halDevice(dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    int ret = 0;

    for (int i=0 ; i < NUM_BAYER_BUFFERS ; i++) {
        m_bayerBufStatus[i] = 0;
        m_bayerDequeueList[i] = -1;
    }
    for (int i=0 ; i < NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY ; i++) {
        m_bayerQueueList[i] = -1;
        m_bayerQueueRequestList[i] = -1;
    }
    m_exynosPictureCSC = NULL;

    if (!m_grallocHal) {
        ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal);
        if (ret)
            ALOGE("ERR(%s):Fail on loading gralloc HAL", __FUNCTION__);
    }

    m_ionCameraClient = createIonClient(m_ionCameraClient);
    if(m_ionCameraClient == 0)
        ALOGE("ERR(%s):Fail on ion_client_create", __FUNCTION__);

    m_mainThread    = new MainThread(this);
    m_sensorThread  = new SensorThread(this);
    m_ispThread     = new IspThread(this);
    m_mainThread->Start("MainThread", PRIORITY_DEFAULT, 0);
    ALOGV("DEBUG(%s): created sensorthread ################", __FUNCTION__);
    usleep(1600000);

    ALOGV("DEBUG(%s): sleep end            ################", __FUNCTION__);
    m_requestManager = new RequestManager((SignalDrivenThread*)(m_mainThread.get()));
    CSC_METHOD cscMethod = CSC_METHOD_HW;
    m_exynosPictureCSC = csc_init(cscMethod);
    if (m_exynosPictureCSC == NULL)
        ALOGE("ERR(%s): csc_init() fail", __FUNCTION__);
    csc_set_hw_property(m_exynosPictureCSC, CSC_HW_PROPERTY_FIXED_NODE, PICTURE_GSC_NODE_NUM);

    ALOGV("DEBUG(%s): END", __FUNCTION__);
}

ExynosCameraHWInterface2::~ExynosCameraHWInterface2()
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    this->release();
}

void ExynosCameraHWInterface2::release()
{
    int i, res;
    ALOGV("DEBUG(%s):", __func__);
    m_closing = true;
    if (m_ispThread != NULL) {
        m_ispThread->release();
        m_ispThread->requestExitAndWait();
        ALOGV("DEBUG(%s):Release ISPthread Done", __func__);
        m_ispThread = NULL;
    }

    if (m_sensorThread != NULL) {
        m_sensorThread->release();
        m_sensorThread->requestExitAndWait();
        ALOGV("DEBUG(%s):Release Sensorthread Done", __func__);
        m_sensorThread = NULL;
    }

    if (m_mainThread != NULL) {
        m_mainThread->release();
        m_mainThread->requestExitAndWait();
        ALOGV("DEBUG(%s):Release Mainthread Done", __func__);
        m_mainThread = NULL;
    }

    if (m_streamThreads[0] != NULL) {
        m_streamThreads[0]->release();
        m_streamThreads[0]->requestExitAndWait();
        ALOGV("DEBUG(%s):Release streamThread[0] Done", __FUNCTION__);
        m_streamThreads[0] = NULL;
    }

    if (m_streamThreads[1] != NULL) {
        m_streamThreads[1]->release();
        m_streamThreads[1]->requestExitAndWait();
        ALOGV("DEBUG(%s):Release streamThread[1] Done", __FUNCTION__);
        m_streamThreads[1] = NULL;
    }


    if (m_exynosPictureCSC)
        csc_deinit(m_exynosPictureCSC);
    m_exynosPictureCSC = NULL;

    for(i = 0; i < m_camera_info.sensor.buffers; i++)
        freeCameraMemory(&m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);

    for(i = 0; i < m_camera_info.capture.buffers; i++)
        freeCameraMemory(&m_camera_info.capture.buffer[i], m_camera_info.capture.planes);

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - sensor", __func__);
    res = exynos_v4l2_close(m_camera_info.sensor.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__func__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - isp", __func__);
    res = exynos_v4l2_close(m_camera_info.isp.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__func__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - capture", __func__);
    res = exynos_v4l2_close(m_camera_info.capture.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__func__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - scp", __func__);
    res = exynos_v4l2_close(m_fd_scp); // HACK
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__func__ , res);
    }
    ALOGV("DEBUG(%s): calling deleteIonClient", __func__);
    deleteIonClient(m_ionCameraClient);
    ALOGV("DEBUG(%s): DONE", __func__);
}

int ExynosCameraHWInterface2::getCameraId() const
{
    return 0;
}

int ExynosCameraHWInterface2::setRequestQueueSrcOps(const camera2_request_queue_src_ops_t *request_src_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    if ((NULL != request_src_ops) && (NULL != request_src_ops->dequeue_request)
            && (NULL != request_src_ops->free_request) && (NULL != request_src_ops->request_count)) {
        m_requestQueueOps = (camera2_request_queue_src_ops_t*)request_src_ops;
        return 0;
    }
    else {
        ALOGE("DEBUG(%s):setRequestQueueSrcOps : NULL arguments", __FUNCTION__);
        return 1;
    }
}

int ExynosCameraHWInterface2::notifyRequestQueueNotEmpty()
{
    ALOGV("DEBUG(%s):setting [SIGNAL_MAIN_REQ_Q_NOT_EMPTY]", __FUNCTION__);
    if ((NULL==m_frameQueueOps)|| (NULL==m_requestQueueOps)) {
        ALOGE("DEBUG(%s):queue ops NULL. ignoring request", __FUNCTION__);
        return 0;
    }
    m_isRequestQueueNull = false;
    m_mainThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
    return 0;
}

int ExynosCameraHWInterface2::setFrameQueueDstOps(const camera2_frame_queue_dst_ops_t *frame_dst_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    if ((NULL != frame_dst_ops) && (NULL != frame_dst_ops->dequeue_frame)
            && (NULL != frame_dst_ops->cancel_frame) && (NULL !=frame_dst_ops->enqueue_frame)) {
        m_frameQueueOps = (camera2_frame_queue_dst_ops_t *)frame_dst_ops;
        return 0;
    }
    else {
        ALOGE("DEBUG(%s):setFrameQueueDstOps : NULL arguments", __FUNCTION__);
        return 1;
    }
}

int ExynosCameraHWInterface2::getInProgressCount()
{
    int inProgressCount = m_requestManager->GetNumEntries();
    ALOGV("DEBUG(%s): # of dequeued req (%d)", __FUNCTION__, inProgressCount);
    return inProgressCount;
}

int ExynosCameraHWInterface2::flushCapturesInProgress()
{
    return 0;
}

int ExynosCameraHWInterface2::constructDefaultRequest(int request_template, camera_metadata_t **request)
{
    ALOGV("DEBUG(%s): making template (%d) ", __FUNCTION__, request_template);

    if (request == NULL) return BAD_VALUE;
    if (request_template < 0 || request_template >= CAMERA2_TEMPLATE_COUNT) {
        return BAD_VALUE;
    }
    status_t res;
    // Pass 1, calculate size and allocate
    res = constructDefaultRequestInternal(request_template,
            request,
            true);
    if (res != OK) {
        return res;
    }
    // Pass 2, build request
    res = constructDefaultRequestInternal(request_template,
            request,
            false);
    if (res != OK) {
        ALOGE("Unable to populate new request for template %d",
                request_template);
    }

    return res;
}

int ExynosCameraHWInterface2::allocateStream(uint32_t width, uint32_t height, int format, const camera2_stream_ops_t *stream_ops,
                                    uint32_t *stream_id, uint32_t *format_actual, uint32_t *usage, uint32_t *max_buffers)
{
    ALOGD("DEBUG(%s): allocate stream width(%d) height(%d) format(%x)", __FUNCTION__,  width, height, format);
    char node_name[30];
    int fd = 0;
    StreamThread *AllocatedStream;
    stream_parameters_t newParameters;

    if (format == CAMERA2_HAL_PIXEL_FORMAT_OPAQUE && width==1920 && height==1080) {

        *stream_id = 0;

        m_streamThreads[0]  = new StreamThread(this, *stream_id);
        AllocatedStream = (StreamThread*)(m_streamThreads[0].get());
        memset(&node_name, 0x00, sizeof(char[30]));
        sprintf(node_name, "%s%d", NODE_PREFIX, 44);
        fd = exynos_v4l2_open(node_name, O_RDWR, 0);
        if (fd < 0) {
            ALOGE("DEBUG(%s): failed to open preview video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
        }
        else {
            ALOGV("DEBUG(%s): preview video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
        }
        m_fd_scp = fd; // HACK

        usleep(100000); // TODO : guarantee the codes below will be run after readyToRunInternal()

        *format_actual = HAL_PIXEL_FORMAT_YV12;
        *usage = GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_YUV_ADDR;
        *max_buffers = 8;

        newParameters.streamType    = 0;
        newParameters.outputWidth   = width;
        newParameters.outputHeight  = height;
        newParameters.nodeWidth     = width;
        newParameters.nodeHeight    = height;
        newParameters.outputFormat  = *format_actual;
        newParameters.nodeFormat    = HAL_PIXEL_FORMAT_2_V4L2_PIX(*format_actual);
        newParameters.streamOps     = stream_ops;
        newParameters.usage         = *usage;
        newParameters.numHwBuffers  = *max_buffers;
        newParameters.fd            = fd;
        newParameters.nodePlanes    = 3;
        newParameters.svcPlanes     = 3;
        newParameters.halBuftype    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        newParameters.memory        = V4L2_MEMORY_DMABUF;

        AllocatedStream->setParameter(&newParameters);
        return 0;
    }
    else if (format == HAL_PIXEL_FORMAT_BLOB) {

        *stream_id = 1;

        m_streamThreads[1]  = new StreamThread(this, *stream_id);
        AllocatedStream = (StreamThread*)(m_streamThreads[1].get());
/*
        memset(&node_name, 0x00, sizeof(char[30]));
        sprintf(node_name, "%s%d", NODE_PREFIX, 42);
        fd = exynos_v4l2_open(node_name, O_RDWR, 0);
        if (fd < 0) {
            ALOGE("DEBUG(%s): failed to open capture video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
        }
        else {
            ALOGV("DEBUG(%s): capture video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
        }
*/
        fd = m_camera_info.capture.fd;
        usleep(100000); // TODO : guarantee the codes below will be run after readyToRunInternal()

        *format_actual = HAL_PIXEL_FORMAT_BLOB;

        *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
        *max_buffers = 8;

        newParameters.streamType    = 1;
        newParameters.outputWidth   = width;
        newParameters.outputHeight  = height;
        newParameters.nodeWidth     = 2560;
        newParameters.nodeHeight    = 1920;
        newParameters.outputFormat  = *format_actual;
        newParameters.nodeFormat    = V4L2_PIX_FMT_YUYV;
        newParameters.streamOps     = stream_ops;
        newParameters.usage         = *usage;
        newParameters.numHwBuffers  = *max_buffers;
        newParameters.fd            = fd;
        newParameters.nodePlanes    = 1;
        newParameters.svcPlanes     = 1;
        newParameters.halBuftype    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        newParameters.memory        = V4L2_MEMORY_DMABUF;
        newParameters.ionClient     = m_ionCameraClient;

        AllocatedStream->setParameter(&newParameters);
        return 0;
    }
    ALOGE("DEBUG(%s): Unsupported Pixel Format", __FUNCTION__);
    return 1; // TODO : check proper error code
}

int ExynosCameraHWInterface2::registerStreamBuffers(uint32_t stream_id,
        int num_buffers, buffer_handle_t *registeringBuffers)
{
    int                     i,j;
    void                    *virtAddr[3];
    uint32_t                plane_index = 0;
    stream_parameters_t     *targetStreamParms;
    node_info_t             *currentNode;

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    ALOGV("DEBUG(%s): streamID (%d), num_buff(%d), handle(%x) ", __FUNCTION__,
        stream_id, num_buffers, (uint32_t)registeringBuffers);

    if (stream_id == 0) {
        targetStreamParms = &(m_streamThreads[0]->m_parameters);
    }
    else if (stream_id == 1) {
        targetStreamParms = &(m_streamThreads[1]->m_parameters);
    }
    else {
        ALOGE("ERR(%s) unregisterd stream id (%d)", __FUNCTION__, stream_id);
        return 1; // TODO : proper error code?
    }

    if (targetStreamParms->streamType ==0) {
        if (num_buffers < targetStreamParms->numHwBuffers) {
            ALOGE("ERR(%s) registering insufficient num of buffers (%d) < (%d)",
                __FUNCTION__, num_buffers, targetStreamParms->numHwBuffers);
            return 1; // TODO : proper error code?
        }
    }
    ALOGV("DEBUG(%s): format(%x) width(%d), height(%d) svcPlanes(%d)",
            __FUNCTION__, targetStreamParms->outputFormat, targetStreamParms->outputWidth,
            targetStreamParms->outputHeight, targetStreamParms->svcPlanes);

    targetStreamParms->numSvcBuffers = num_buffers;
    currentNode = &(targetStreamParms->node); // TO Remove

    currentNode->fd         = targetStreamParms->fd;
    currentNode->width      = targetStreamParms->nodeWidth;
    currentNode->height     = targetStreamParms->nodeHeight;
    currentNode->format     = targetStreamParms->nodeFormat;
    currentNode->planes     = targetStreamParms->nodePlanes;
    currentNode->buffers    = targetStreamParms->numHwBuffers;
    currentNode->type       = targetStreamParms->halBuftype;
    currentNode->memory     = targetStreamParms->memory;
    currentNode->ionClient  = targetStreamParms->ionClient;

    if (targetStreamParms->streamType == 0) {
        cam_int_s_input(currentNode, m_camera_info.sensor_id);
        cam_int_s_fmt(currentNode);
        cam_int_reqbufs(currentNode);
    }
    else if (targetStreamParms->streamType == 1) {
        for(i = 0; i < currentNode->buffers; i++){
            memcpy(&(currentNode->buffer[i]), &(m_camera_info.capture.buffer[i]), sizeof(ExynosBuffer));
        }
    }

    for (i = 0 ; i<targetStreamParms->numSvcBuffers ; i++) {
        ALOGV("DEBUG(%s): registering Stream Buffers[%d] (%x) ", __FUNCTION__,
            i, (uint32_t)(registeringBuffers[i]));
        if (m_grallocHal) {
            if (m_grallocHal->lock(m_grallocHal, registeringBuffers[i],
                   targetStreamParms->usage, 0, 0,
                   currentNode->width, currentNode->height, virtAddr) != 0) {
                ALOGE("ERR(%s): could not obtain gralloc buffer", __FUNCTION__);
            }
            else {
                v4l2_buf.m.planes   = planes;
                v4l2_buf.type       = currentNode->type;
                v4l2_buf.memory     = currentNode->memory;
                v4l2_buf.index      = i;
                v4l2_buf.length     = currentNode->planes;

                ExynosBuffer currentBuf;
                const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(registeringBuffers[i]);

                m_getAlignedYUVSize(currentNode->format,
                    currentNode->width, currentNode->height, &currentBuf);

                    v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                    v4l2_buf.m.planes[2].m.fd = priv_handle->u_fd;
                    v4l2_buf.m.planes[1].m.fd = priv_handle->v_fd;
                for (plane_index=0 ; plane_index < v4l2_buf.length ; plane_index++) {
//                    v4l2_buf.m.planes[plane_index].m.userptr = (unsigned long)(virtAddr[plane_index]);
                    currentBuf.virt.extP[plane_index] = (char *)virtAddr[plane_index];
                    v4l2_buf.m.planes[plane_index].length  = currentBuf.size.extS[plane_index];
                    ALOGV("DEBUG(%s): plane(%d): fd(%d) addr(%x), length(%d)",
                         __FUNCTION__, plane_index, v4l2_buf.m.planes[plane_index].m.fd,
                         (unsigned int)currentBuf.virt.extP[plane_index],
                         v4l2_buf.m.planes[plane_index].length);
                }

                if (targetStreamParms->streamType == 0) {
                    if (i < currentNode->buffers) {
                        if (exynos_v4l2_qbuf(currentNode->fd, &v4l2_buf) < 0) {
                            ALOGE("ERR(%s): stream id(%d) exynos_v4l2_qbuf() fail",
                                __FUNCTION__, stream_id);
                            return false;
                        }
                        targetStreamParms->svcBufStatus[i]  = REQUIRES_DQ_FROM_SVC;
                    }
                    else {
                        targetStreamParms->svcBufStatus[i]  = ON_SERVICE;
                    }
                }
                else if (targetStreamParms->streamType == 1) {
                    targetStreamParms->svcBufStatus[i]  = ON_SERVICE;
                }
                targetStreamParms->svcBuffers[i]       = currentBuf;
                targetStreamParms->svcBufHandle[i]     = registeringBuffers[i];
            }
        }
    }

    ALOGV("DEBUG(%s): END registerStreamBuffers", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::releaseStream(uint32_t stream_id)
{
    StreamThread *targetStream;
    ALOGV("DEBUG(%s):", __FUNCTION__);

    if (stream_id==0) {
        targetStream = (StreamThread*)(m_streamThreads[0].get());
    }
    else if (stream_id==1) {
        targetStream = (StreamThread*)(m_streamThreads[1].get());
    }
    else {
        ALOGE("ERR:(%s): wrong stream id (%d)", __FUNCTION__, stream_id);
        return 1; // TODO : proper error code?
    }

    targetStream->release();
    ALOGV("DEBUG(%s): DONE", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::allocateReprocessStream(
    uint32_t width, uint32_t height, uint32_t format,
    const camera2_stream_in_ops_t *reprocess_stream_ops,
    uint32_t *stream_id, uint32_t *consumer_usage, uint32_t *max_buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::releaseReprocessStream(uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::triggerAction(uint32_t trigger_id, int ext1, int ext2)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::setNotifyCallback(camera2_notify_callback notify_cb, void *user)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    m_notifyCb = notify_cb;
    m_callbackCookie = user;
    return 0;
}

int ExynosCameraHWInterface2::getMetadataVendorTagOps(vendor_tag_query_ops_t **ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::dump(int fd)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

void ExynosCameraHWInterface2::m_getAlignedYUVSize(int colorFormat, int w, int h, ExynosBuffer *buf)
{
    switch (colorFormat) {
    // 1p
    case V4L2_PIX_FMT_RGB565 :
    case V4L2_PIX_FMT_YUYV :
    case V4L2_PIX_FMT_UYVY :
    case V4L2_PIX_FMT_VYUY :
    case V4L2_PIX_FMT_YVYU :
        buf->size.extS[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(colorFormat), w, h);
        buf->size.extS[1] = 0;
        buf->size.extS[2] = 0;
        break;
    // 2p
    case V4L2_PIX_FMT_NV12 :
    case V4L2_PIX_FMT_NV12T :
    case V4L2_PIX_FMT_NV21 :
        buf->size.extS[0] = ALIGN(w,   16) * ALIGN(h,   16);
        buf->size.extS[1] = ALIGN(w/2, 16) * ALIGN(h/2, 16);
        buf->size.extS[2] = 0;
        break;
    case V4L2_PIX_FMT_NV12M :
    case V4L2_PIX_FMT_NV12MT_16X16 :
        buf->size.extS[0] = ALIGN(w, 16) * ALIGN(h,     16);
        buf->size.extS[1] = ALIGN(buf->size.extS[0] / 2, 256);
        buf->size.extS[2] = 0;
        break;
    case V4L2_PIX_FMT_NV16 :
    case V4L2_PIX_FMT_NV61 :
        buf->size.extS[0] = ALIGN(w, 16) * ALIGN(h, 16);
        buf->size.extS[1] = ALIGN(w, 16) * ALIGN(h,  16);
        buf->size.extS[2] = 0;
        break;
     // 3p
    case V4L2_PIX_FMT_YUV420 :
    case V4L2_PIX_FMT_YVU420 :
        buf->size.extS[0] = (w * h);
        buf->size.extS[1] = (w * h) >> 2;
        buf->size.extS[2] = (w * h) >> 2;
        break;
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YVU420M :
    case V4L2_PIX_FMT_YUV422P :
        buf->size.extS[0] = ALIGN(w,  32) * ALIGN(h,  16);
        buf->size.extS[1] = ALIGN(w/2, 16) * ALIGN(h/2, 8);
        buf->size.extS[2] = ALIGN(w/2, 16) * ALIGN(h/2, 8);
        break;
    default:
        ALOGE("ERR(%s):unmatched colorFormat(%d)", __FUNCTION__, colorFormat);
        return;
        break;
    }
}

bool ExynosCameraHWInterface2::m_getRatioSize(int  src_w,  int   src_h,
                                             int  dst_w,  int   dst_h,
                                             int *crop_x, int *crop_y,
                                             int *crop_w, int *crop_h,
                                             int zoom)
{
    *crop_w = src_w;
    *crop_h = src_h;

    if (   src_w != dst_w
        || src_h != dst_h) {
        float src_ratio = 1.0f;
        float dst_ratio = 1.0f;

        // ex : 1024 / 768
        src_ratio = (float)src_w / (float)src_h;

        // ex : 352  / 288
        dst_ratio = (float)dst_w / (float)dst_h;

        if (dst_w * dst_h < src_w * src_h) {
            if (dst_ratio <= src_ratio) {
                // shrink w
                *crop_w = src_h * dst_ratio;
                *crop_h = src_h;
            } else {
                // shrink h
                *crop_w = src_w;
                *crop_h = src_w / dst_ratio;
            }
        } else {
            if (dst_ratio <= src_ratio) {
                // shrink w
                *crop_w = src_h * dst_ratio;
                *crop_h = src_h;
            } else {
                // shrink h
                *crop_w = src_w;
                *crop_h = src_w / dst_ratio;
            }
        }
    }

    if (zoom != 0) {
        float zoomLevel = ((float)zoom + 10.0) / 10.0;
        *crop_w = (int)((float)*crop_w / zoomLevel);
        *crop_h = (int)((float)*crop_h / zoomLevel);
    }

    #define CAMERA_CROP_WIDTH_RESTRAIN_NUM  (0x2)
    unsigned int w_align = (*crop_w & (CAMERA_CROP_WIDTH_RESTRAIN_NUM - 1));
    if (w_align != 0) {
        if (  (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1) <= w_align
            && *crop_w + (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align) <= dst_w) {
            *crop_w += (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align);
        }
        else
            *crop_w -= w_align;
    }

    #define CAMERA_CROP_HEIGHT_RESTRAIN_NUM  (0x2)
    unsigned int h_align = (*crop_h & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - 1));
    if (h_align != 0) {
        if (  (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1) <= h_align
            && *crop_h + (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align) <= dst_h) {
            *crop_h += (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align);
        }
        else
            *crop_h -= h_align;
    }

    *crop_x = (src_w - *crop_w) >> 1;
    *crop_y = (src_h - *crop_h) >> 1;

    if (*crop_x & (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1))
        *crop_x -= 1;

    if (*crop_y & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1))
        *crop_y -= 1;

    return true;
}

void ExynosCameraHWInterface2::RegisterBayerQueueList(int bufIndex, int requestIndex)
{
    if (m_bayerQueueList[m_numBayerQueueList+m_numBayerQueueListRemainder]!=-1) {
        ALOGD("DBG(%s): entry(%d) not empty (%d, %d)", __FUNCTION__,
            m_numBayerQueueList, m_bayerQueueList[m_numBayerQueueList+m_numBayerQueueListRemainder],
            m_bayerQueueRequestList[m_numBayerQueueList+m_numBayerQueueListRemainder]);
        return;
    }
    m_bayerQueueList[m_numBayerQueueList+m_numBayerQueueListRemainder] = bufIndex;
    m_bayerQueueRequestList[m_numBayerQueueList+m_numBayerQueueListRemainder] = requestIndex;
    m_numBayerQueueList++;
    ALOGV("DEBUG(%s) END: bufIndex(%d) requestIndex(%d) - # of current entry(%d)",
        __FUNCTION__, bufIndex, requestIndex, m_numBayerQueueList);
#if 0
    for (int i=0 ; i < NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY ; i++) {
        ALOGV("DEBUG(%s): QueuedEntry[%d] <bufIndex(%d) Request(%d)>", __FUNCTION__,
            i, m_bayerQueueList[i], m_bayerQueueRequestList[i]);
    }
#endif
}

void ExynosCameraHWInterface2::DeregisterBayerQueueList(int bufIndex)
{
    ALOGV("DEBUG(%s): deregistering bufIndex(%d)", __FUNCTION__, bufIndex);
    int i, j;
    for (int i=0 ; i<NUM_BAYER_BUFFERS ; i++) {
        if (m_bayerQueueList[i]==-1) {
            if (m_bayerQueueRequestList[i]==-1) {
                //ALOGE("ERR(%s): abnormal - entry(%d) should not empty", __FUNCTION__, i);
            }
            else {
                ALOGV("DEBUG(%s): entry(%d) has remainder request(%d)",
                    __FUNCTION__, i, m_bayerQueueRequestList[i]);
                continue;
            }
        }
        if (m_bayerQueueList[i]==bufIndex) {
            if (m_bayerQueueRequestList[i]==-1 && i==0) {
                ALOGV("DEBUG(%s): removing entry(%d)", __FUNCTION__, i);
                for (j=i ; j < NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY-1 ; j++) {
                    m_bayerQueueList[j] = m_bayerQueueList[j+1];
                    m_bayerQueueRequestList[j] = m_bayerQueueRequestList[j+1];
                }
                m_bayerQueueList[j] = -1;
                m_bayerQueueRequestList[j] = -1;
            }
            else {
                ALOGV("DEBUG(%s): entry(%d) is now remainder request(%d)",
                    __FUNCTION__, i, m_bayerQueueRequestList[i]);
                m_bayerQueueList[i] = -1;
                m_numBayerQueueListRemainder++;
            }
            m_numBayerQueueList--;
            break;
        }
    }
    ALOGV("DEBUG(%s): numQueueList(%d), remainder(%d)", __FUNCTION__,
                m_numBayerQueueList,m_numBayerQueueListRemainder);
#if 0
    for (int i=0 ; i < NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY ; i++) {
        ALOGV("DEBUG(%s): QueuedEntry[%d] <bufIndex(%d) Request(%d)>", __FUNCTION__,
            i, m_bayerQueueList[i], m_bayerQueueRequestList[i]);
    }
#endif
}


void ExynosCameraHWInterface2::RegisterBayerDequeueList(int bufIndex)
{
    if (m_bayerDequeueList[m_numBayerDequeueList]!=-1) {
        ALOGD("DBG(%s): entry(%d) not empty (%d)", __FUNCTION__,
            m_numBayerDequeueList, m_bayerDequeueList[m_numBayerDequeueList]);
        return;
    }
    m_bayerDequeueList[m_numBayerDequeueList] = bufIndex;
    m_numBayerDequeueList++;
    ALOGV("DEBUG(%s) END: bufIndex(%d) - # of current entry(%d)",
        __FUNCTION__, bufIndex, m_numBayerDequeueList);
}


int ExynosCameraHWInterface2::DeregisterBayerDequeueList(void)
{
    ALOGV("DEBUG(%s): deregistering a buf, curr num(%d)", __FUNCTION__, m_numBayerDequeueList);
    int ret = m_bayerDequeueList[0];
    int i = 0;
    if (m_numBayerDequeueList == 0) {
        ALOGV("DEBUG(%s): no bayer buffer to deregister", __FUNCTION__);
        return -1;
    }

    for (i=0; i < NUM_BAYER_BUFFERS-1 ; i++) {
        m_bayerDequeueList[i] = m_bayerDequeueList[i+1];
    }
    m_bayerDequeueList[i] = -1;
    m_numBayerDequeueList--;
    ALOGV("DEBUG(%s) END: deregistered buf(%d), curr num(%d)", __FUNCTION__,
        ret, m_numBayerDequeueList);

#if 0
    for (i=0 ; i < NUM_BAYER_BUFFERS ; i++) {
        ALOGV("DEBUG(%s): QueuedEntry[%d] <bufIndex(%d)>", __FUNCTION__,
            i, m_bayerDequeueList[i]);
    }
#endif
    return ret;
}


int ExynosCameraHWInterface2::FindRequestEntryNumber(int bufIndex)
{
    bool found = false;
    ALOGV("DEBUG(%s): finding entry# for bufindex(%d)", __FUNCTION__, bufIndex);
    int i, j, ret;
    // if driver supports shot mumber matching, just compare shot number
#if 1
    if (SHOT_FRAME_DELAY>m_numBayerQueueList+m_numBayerQueueListRemainder) {
        ALOGE("ERR(%s): abnormal # of entry (%d) + (%d)", __FUNCTION__,
            m_numBayerQueueList, m_numBayerQueueListRemainder);
        return -1;
    }

    ALOGV("DEBUG(%s): numQueueList(%d), remainder(%d)", __FUNCTION__,
                m_numBayerQueueList,m_numBayerQueueListRemainder);
    for (i=0 ; i < NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY ; i++) {
        ALOGV("DEBUG(%s): QueuedEntry[%2d] <bufIndex(%3d) Request(%3d)>", __FUNCTION__,
            i, m_bayerQueueList[i], m_bayerQueueRequestList[i]);
    }

    for (i=0 ; i<=(m_numBayerQueueList+m_numBayerQueueListRemainder); i++) {
        if (m_bayerQueueList[i]==bufIndex) {
            found = true;
            break;
        }
    }
    if (found) {
        ALOGV("DEBUG(%s): found (%d) at Queue entry [%d]",
        __FUNCTION__, bufIndex, i);
        if (i != SHOT_FRAME_DELAY-1) {
            ALOGV("DEBUG(%s):no match ?? ", __FUNCTION__);
            return -1;
        }
        else {
            ret = m_bayerQueueRequestList[0];
            ALOGV("DEBUG(%s): removing entry[%d]", __FUNCTION__, i);
            for (j=0 ; j < NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY-1 ; j++) {
                m_bayerQueueList[j] = m_bayerQueueList[j+1];
                m_bayerQueueRequestList[j] = m_bayerQueueRequestList[j+1];
            }
            m_bayerQueueList[j] = -1;
            m_bayerQueueRequestList[j] = -1;
            m_numBayerQueueListRemainder--;
            return ret;
        }
    }
    return -1;
#else
    if (SHOT_FRAME_DELAY>m_numBayerQueueList+m_numBayerQueueListRemainder) {
        ALOGE("ERR(%s): abnormal # of entry (%d) + (%d)", __FUNCTION__,
            m_numBayerQueueList, m_numBayerQueueListRemainder);
        return -1;
    }

    for (int i=SHOT_FRAME_DELAY ; i<=(m_numBayerQueueList+m_numBayerQueueListRemainder); i--) {
        if (m_bayerQueueList[i]==bufIndex) {
            ALOGV("DEBUG(%s): found entry number(%d)", __FUNCTION__, m_bayerQueueRequestList[i-SHOT_FRAME_DELAY]);
            ret = m_bayerQueueRequestList[i-SHOT_FRAME_DELAY];
            m_bayerQueueRequestList[i-SHOT_FRAME_DELAY] = -1;
            return ret;
        }
    }
    return -1;

#endif
}

void ExynosCameraHWInterface2::m_mainThreadFunc(SignalDrivenThread * self)
{
    camera_metadata_t *currentRequest = NULL;
    camera_metadata_t *currentFrame = NULL;
    size_t numEntries = 0;
    size_t frameSize = 0;
    camera_metadata_t * preparedFrame = NULL;
    camera_metadata_t *deregisteredRequest = NULL;
    uint32_t currentSignal = self->GetProcessingSignal();
    MainThread *  selfThread      = ((MainThread*)self);
    int res = 0;

    ALOGV("DEBUG(%s): m_mainThreadFunc (%x)", __FUNCTION__, currentSignal);

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE", __FUNCTION__);

        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE DONE", __FUNCTION__);
        selfThread->SetSignal(SIGNAL_THREAD_TERMINATE);
        return;
    }

    if (currentSignal & SIGNAL_MAIN_REQ_Q_NOT_EMPTY) {
        ALOGV("DEBUG(%s): MainThread processing SIGNAL_MAIN_REQ_Q_NOT_EMPTY", __FUNCTION__);
        if (m_requestManager->IsRequestQueueFull()==false
                && m_requestManager->GetNumEntries()<NUM_MAX_DEQUEUED_REQUEST) {
            m_requestQueueOps->dequeue_request(m_requestQueueOps, &currentRequest);
            if (NULL == currentRequest) {
                ALOGV("DEBUG(%s): dequeue_request returned NULL ", __FUNCTION__);
                m_isRequestQueueNull = true;
            }
            else {
                m_requestManager->RegisterRequest(currentRequest);

                m_numOfRemainingReqInSvc = m_requestQueueOps->request_count(m_requestQueueOps);
                ALOGV("DEBUG(%s): remaining req cnt (%d)", __FUNCTION__, m_numOfRemainingReqInSvc);
                if (m_requestManager->IsRequestQueueFull()==false
                    && m_requestManager->GetNumEntries()<NUM_MAX_DEQUEUED_REQUEST)
                    selfThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY); // dequeue repeatedly
                m_sensorThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
            }
        }
        else {
            m_isRequestQueuePending = true;
        }
    }

    if (currentSignal & SIGNAL_MAIN_STREAM_OUTPUT_DONE) {
        ALOGV("DEBUG(%s): MainThread processing SIGNAL_MAIN_STREAM_OUTPUT_DONE", __FUNCTION__);
        /*while (1)*/ {
            m_lastTimeStamp = 0;
            m_requestManager->PrepareFrame(&numEntries, &frameSize, &preparedFrame);
            m_requestManager->DeregisterRequest(&deregisteredRequest);
            m_requestQueueOps->free_request(m_requestQueueOps, deregisteredRequest);
            m_frameQueueOps->dequeue_frame(m_frameQueueOps, numEntries, frameSize, &currentFrame);
            if (currentFrame==NULL) {
                ALOGD("DBG(%s): frame dequeue returned NULL",__FUNCTION__ );
            }
            else {
                ALOGV("DEBUG(%s): frame dequeue done. numEntries(%d) frameSize(%d)",__FUNCTION__ , numEntries,frameSize);
            }
            res = append_camera_metadata(currentFrame, preparedFrame);
            if (res==0) {
                ALOGV("DEBUG(%s): frame metadata append success",__FUNCTION__);
                m_frameQueueOps->enqueue_frame(m_frameQueueOps, currentFrame);
            }
            else {
                ALOGE("ERR(%s): frame metadata append fail (%d)",__FUNCTION__, res);
            }
        }
        if (!m_isRequestQueueNull) {
            selfThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
        }
        // temp code only before removing auto mode
        if (getInProgressCount()>0) {
            ALOGV("DEBUG(%s): STREAM_OUTPUT_DONE and signalling REQ_PROCESSING",__FUNCTION__);
            m_sensorThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
        }
    }
    ALOGV("DEBUG(%s): MainThread Exit", __FUNCTION__);
    return;
}

void ExynosCameraHWInterface2::m_sensorThreadInitialize(SignalDrivenThread * self)
{
    ALOGV("DEBUG(%s): ", __FUNCTION__ );
    SensorThread * selfThread = ((SensorThread*)self);
    char node_name[30];
    int fd = 0;
    int i =0, j=0;

    m_camera_info.sensor_id = SENSOR_NAME_S5K4E5;
    memset(&m_camera_info.dummy_shot, 0x00, sizeof(struct camera2_shot_ext));
	m_camera_info.dummy_shot.shot.ctl.request.metadataMode = METADATA_MODE_FULL;
	m_camera_info.dummy_shot.shot.magicNumber = 0x23456789;

    /*sensor setting*/
	m_camera_info.dummy_shot.shot.ctl.sensor.exposureTime = 0;
	m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 0;
	m_camera_info.dummy_shot.shot.ctl.sensor.sensitivity = 0;

    m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[0] = 0;
    m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[1] = 0;
    m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[2] = 1920;

	/*request setting*/
	m_camera_info.dummy_shot.request_sensor = 1;
	m_camera_info.dummy_shot.request_scc = 0;
	m_camera_info.dummy_shot.request_scp = 0;

    /*sensor init*/
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 40);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open sensor video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): sensor video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.sensor.fd = fd;
    m_camera_info.sensor.width = 2560 + 16;
    m_camera_info.sensor.height = 1920 + 10;
    m_camera_info.sensor.format = V4L2_PIX_FMT_SBGGR16;
    m_camera_info.sensor.planes = 2;
    m_camera_info.sensor.buffers = NUM_BAYER_BUFFERS;
    m_camera_info.sensor.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_camera_info.sensor.memory = V4L2_MEMORY_DMABUF;
    m_camera_info.sensor.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.sensor.buffers; i++){
        initCameraMemory(&m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);
        m_camera_info.sensor.buffer[i].size.extS[0] = m_camera_info.sensor.width*m_camera_info.sensor.height*2;
        m_camera_info.sensor.buffer[i].size.extS[1] = 8*1024; // HACK, driver use 8*1024, should be use predefined value
        allocCameraMemory(m_camera_info.sensor.ionClient, &m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);
    }

    m_initFlag1 = true;

#if 0
    /*isp init*/
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 41);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open isp video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): isp video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.isp.fd = fd;

    m_camera_info.isp.width = m_camera_info.sensor.width;
    m_camera_info.isp.height = m_camera_info.sensor.height;
    m_camera_info.isp.format = m_camera_info.sensor.format;
    m_camera_info.isp.planes = m_camera_info.sensor.planes;
    m_camera_info.isp.buffers = m_camera_info.sensor.buffers;
    m_camera_info.isp.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    m_camera_info.isp.memory = V4L2_MEMORY_DMABUF;
    //m_camera_info.isp.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.isp.buffers; i++){
        initCameraMemory(&m_camera_info.isp.buffer[i], m_camera_info.isp.planes);
        m_camera_info.isp.buffer[i].size.extS[0]    = m_camera_info.sensor.buffer[i].size.extS[0];
        m_camera_info.isp.buffer[i].size.extS[1]    = m_camera_info.sensor.buffer[i].size.extS[1];
        m_camera_info.isp.buffer[i].fd.extFd[0]     = m_camera_info.sensor.buffer[i].fd.extFd[0];
        m_camera_info.isp.buffer[i].fd.extFd[1]     = m_camera_info.sensor.buffer[i].fd.extFd[1];
        m_camera_info.isp.buffer[i].virt.extP[0]    = m_camera_info.sensor.buffer[i].virt.extP[0];
        m_camera_info.isp.buffer[i].virt.extP[1]    = m_camera_info.sensor.buffer[i].virt.extP[1];
    };
    ALOGV("DEBUG(%s): isp mem alloc done",  __FUNCTION__);

#endif
#if 0
    cam_int_s_input(&(m_camera_info.sensor), m_camera_info.sensor_id);
    ALOGV("DEBUG(%s): sensor s_input done",  __FUNCTION__);

    if (cam_int_s_fmt(&(m_camera_info.sensor))< 0) {
        ALOGE("ERR(%s): sensor s_fmt fail",  __FUNCTION__);
    }
    ALOGV("DEBUG(%s): sensor s_fmt done",  __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.sensor));
    ALOGV("DEBUG(%s): sensor reqbuf done",  __FUNCTION__);
    for (i = 0; i < m_camera_info.sensor.buffers; i++) {
        ALOGV("DEBUG(%s): sensor initial QBUF [%d]",  __FUNCTION__, i);
        memcpy( m_camera_info.sensor.buffer[i].virt.extP[1], &(m_camera_info.current_shot),
                sizeof(camera2_shot_ext));
        cam_int_qbuf(&(m_camera_info.sensor), i);
    }
    cam_int_streamon(&(m_camera_info.sensor));
    m_camera_info.sensor.currentBufferIndex = 0;
#endif
#if 0
    cam_int_s_input(&(m_camera_info.isp), m_camera_info.sensor_id);
    cam_int_s_fmt(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling reqbuf", __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling querybuf", __FUNCTION__);

    for (i = 0; i < m_camera_info.isp.buffers; i++) {
        ALOGV("DEBUG(%s): isp initial QBUF [%d]",  __FUNCTION__, i);
        cam_int_qbuf(&(m_camera_info.isp), i);
    }
    cam_int_streamon(&(m_camera_info.isp));

    for (i = 0; i < m_camera_info.isp.buffers; i++) {
        ALOGV("DEBUG(%s): isp initial DQBUF [%d]",  __FUNCTION__, i);
        cam_int_dqbuf(&(m_camera_info.isp));
    }
#endif


    while (!m_initFlag2) // temp
        usleep(100000);
    ALOGV("DEBUG(%s): END of SensorThreadInitialize ", __FUNCTION__);
    return;
}



void ExynosCameraHWInterface2::DumpFrameinfoWithBufIndex(int bufIndex)
{
    bool found = false;
    int i;
    struct camera2_shot_ext *shot_ext;
    for (i=0 ; i < NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY ; i++) {
        if (m_bayerQueueList[i] == bufIndex) {
            found = true;
            break;
        }
    }
    if (!found) {
        ALOGD("DEBUG(%s): dumping bufIndex[%d] not found", __FUNCTION__, bufIndex);
    }
    else {
         ALOGD("DEBUG(%s): bufIndex[%d] found at [%d]. reqIndex=[%d]",
            __FUNCTION__, bufIndex, i, m_bayerQueueRequestList[i]);
         ALOGD("#### info : reqManager ####");
            m_requestManager->DumpInfoWithIndex(m_bayerQueueRequestList[i]);
    }

    ALOGD("#### info : shot on sensorBuffer ####");
	shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[bufIndex].virt.extP[1]);
	DumpInfoWithShot(shot_ext);

}

void ExynosCameraHWInterface2::DumpInfoWithShot(struct camera2_shot_ext * shot_ext)
{
    ALOGV("####  common Section");
    ALOGV("####                 magic(%x) ",
        shot_ext->shot.magicNumber);
    ALOGV("####  ctl Section");
    ALOGV("####     metamode(%d) exposureTime(%lld) duration(%lld) ISO(%d) ",
        shot_ext->shot.ctl.request.metadataMode,
        shot_ext->shot.ctl.sensor.exposureTime,
        shot_ext->shot.ctl.sensor.frameDuration,
        shot_ext->shot.ctl.sensor.sensitivity);

    ALOGV("####                 OutputStream Sensor(%d) SCP(%d) SCC(%d)",shot_ext->request_sensor,
        shot_ext->request_scp, shot_ext->request_scc);

    ALOGV("####  DM Section");
    ALOGV("####     metamode(%d) exposureTime(%lld) duration(%lld) ISO(%d) frameCnt(%d) timestamp(%lld)",
        shot_ext->shot.dm.request.metadataMode,
        shot_ext->shot.dm.sensor.exposureTime,
        shot_ext->shot.dm.sensor.frameDuration,
        shot_ext->shot.dm.sensor.sensitivity,
        shot_ext->shot.dm.sensor.frameCount,
        shot_ext->shot.dm.sensor.timeStamp);
}

void ExynosCameraHWInterface2::m_sensorThreadFunc(SignalDrivenThread * self)
{
    uint32_t        currentSignal = self->GetProcessingSignal();
    SensorThread *  selfThread      = ((SensorThread*)self);
    int index;
    status_t res;
    nsecs_t frameTime;
    int bayersOnSensor = 0, bayersOnIsp = 0;
    ALOGV("DEBUG(%s): m_sensorThreadFunc (%x)", __FUNCTION__, currentSignal);

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE", __FUNCTION__);

        for (int i = 0 ; i < NUM_BAYER_BUFFERS ;  i++) {
            ALOGV("DEBUG(%s):###  Bayer Buf[%d] Status (%d)", __FUNCTION__, i, m_bayerBufStatus[i]);
            if (m_bayerBufStatus[i]==BAYER_ON_SENSOR) {
                bayersOnSensor++;
            }
            else if (m_bayerBufStatus[i]==BAYER_ON_ISP) {
                bayersOnIsp++;
            }
        }
        for (int i = 0 ; i < bayersOnSensor ; i++) {
            index = cam_int_dqbuf(&(m_camera_info.sensor));
            ALOGV("DEBUG(%s):###  sensor dqbuf done index(%d)", __FUNCTION__, index);
            m_bayerBufStatus[index] = BAYER_ON_HAL_EMPTY;
        }
        for (int i = 0 ; i < bayersOnIsp ; i++) {
            index = cam_int_dqbuf(&(m_camera_info.isp));
            ALOGV("DEBUG(%s):###  isp dqbuf done index(%d)", __FUNCTION__, index);
            m_bayerBufStatus[index] = BAYER_ON_HAL_EMPTY;
        }

        for (int i = 0 ; i < NUM_BAYER_BUFFERS ;  i++) {
            ALOGV("DEBUG(%s):###  Bayer Buf[%d] Status (%d)", __FUNCTION__, i, m_bayerBufStatus[i]);
        }
        exynos_v4l2_s_ctrl(m_camera_info.sensor.fd, V4L2_CID_IS_S_STREAM, IS_DISABLE_STREAM);
        ALOGV("DEBUG(%s): calling sensor streamoff", __FUNCTION__);
        cam_int_streamoff(&(m_camera_info.sensor));
        ALOGV("DEBUG(%s): calling sensor streamoff done", __FUNCTION__);
        exynos_v4l2_s_ctrl(m_camera_info.sensor.fd, V4L2_CID_IS_S_STREAM, IS_DISABLE_STREAM);
        /*
        ALOGV("DEBUG(%s): calling sensor s_ctrl done", __FUNCTION__);
        m_camera_info.sensor.buffers = 0;
        cam_int_reqbufs(&(m_camera_info.sensor));
        ALOGV("DEBUG(%s): calling sensor reqbuf 0 done", __FUNCTION__);
        */
/*
        ALOGV("DEBUG(%s): calling exynos_v4l2_close - sensor", __FUNCTION__);
        res = exynos_v4l2_close(m_camera_info.sensor.fd);
        if (res != NO_ERROR ) {
            ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
        }
  */
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE DONE", __FUNCTION__);

        selfThread->SetSignal(SIGNAL_THREAD_TERMINATE);
        return;
    }

    if (currentSignal & SIGNAL_SENSOR_START_REQ_PROCESSING)
    {
        ALOGV("DEBUG(%s): SensorThread processing SIGNAL_SENSOR_START_REQ_PROCESSING", __FUNCTION__);
        int targetStreamIndex = 0;
        int matchedEntryNumber, processingReqIndex;
        struct camera2_shot_ext *shot_ext;
        if (!m_isSensorStarted)
        {
            m_isSensorStarted = true;
            ALOGV("DEBUG(%s): calling preview streamon", __FUNCTION__);
            cam_int_streamon(&(m_streamThreads[0]->m_parameters.node));
            ALOGV("DEBUG(%s): calling preview streamon done", __FUNCTION__);
            exynos_v4l2_s_ctrl(m_camera_info.isp.fd, V4L2_CID_IS_S_STREAM, IS_ENABLE_STREAM);
            ALOGV("DEBUG(%s): calling isp sctrl done", __FUNCTION__);
            exynos_v4l2_s_ctrl(m_camera_info.sensor.fd, V4L2_CID_IS_S_STREAM, IS_ENABLE_STREAM);
            ALOGV("DEBUG(%s): calling sensor sctrl done", __FUNCTION__);

        }
        else
        {
            ALOGV("DEBUG(%s): sensor started already", __FUNCTION__);
        }

        ALOGV("### Sensor DQBUF start");
        index = cam_int_dqbuf(&(m_camera_info.sensor));
        frameTime = systemTime();
        ALOGV("### Sensor DQBUF done index(%d)", index);

        if (m_lastTimeStamp!=0 && (frameTime-m_lastTimeStamp)>100000000) {
            ALOGV("########## lost frame detected ########");
            m_lastTimeStamp = 0;
        }
        if (m_bayerBufStatus[index]!=BAYER_ON_SENSOR)
            ALOGD("DBG(%s): bayer buf status abnormal index[%d] status(%d)",
                __FUNCTION__, index, m_bayerBufStatus[index]);

        matchedEntryNumber = FindRequestEntryNumber(index);
        DeregisterBayerQueueList(index);

        if (m_ispInputIndex != -1) {
            ALOGV("####### sensor delay sleep");
            usleep(5000);
        }
        if (matchedEntryNumber != -1) {
            m_bayerBufStatus[index] = BAYER_ON_HAL_FILLED;
            m_ispInputIndex = index;
            m_processingRequest = matchedEntryNumber;
            m_requestManager->RegisterTimestamp(m_processingRequest, &frameTime);
            ALOGD("### Sensor DQed buf index(%d) passing to ISP. req(%d) timestamp(%lld)", index,matchedEntryNumber, frameTime);
            if (!(m_ispThread.get())) return;
            m_ispThread->SetSignal(SIGNAL_ISP_START_BAYER_INPUT);
            //RegisterBayerDequeueList(index);  this will be done in ispthread
        }
        else {
            m_bayerBufStatus[index] = BAYER_ON_HAL_FILLED;
            m_ispInputIndex = index;
            m_processingRequest = -1;
            ALOGV("### Sensor DQed buf index(%d) passing to ISP. BUBBLE", index);
            if (!(m_ispThread.get())) return;
            m_ispThread->SetSignal(SIGNAL_ISP_START_BAYER_INPUT);
            //RegisterBayerDequeueList(index);
        }

        while (m_numBayerQueueList<SHOT_FRAME_DELAY) {

            index = DeregisterBayerDequeueList();
            if (index == -1) {
                ALOGE("ERR(%s) No free Bayer buffer", __FUNCTION__);
                break;
            }
            processingReqIndex = m_requestManager->MarkProcessingRequest(&(m_camera_info.sensor.buffer[index]));

            if (processingReqIndex == -1) {
                ALOGV("DEBUG(%s) req underrun => inserting bubble to index(%d)", __FUNCTION__, index);
          		shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);
                memcpy(shot_ext, &(m_camera_info.dummy_shot), sizeof(struct camera2_shot_ext));
            }

            RegisterBayerQueueList(index, processingReqIndex);

            ALOGV("### Sensor QBUF start index(%d)", index);
            /* if (processingReqIndex != -1)
                DumpFrameinfoWithBufIndex(index); */
            cam_int_qbuf(&(m_camera_info.sensor), index);
            m_bayerBufStatus[index] = BAYER_ON_SENSOR;
            ALOGV("### Sensor QBUF done");
        }
        if (!m_closing) selfThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
        return;
#if 0
        if (m_numBayerQueueList==3) {
            selfThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
            ALOGV("### Sensor will not QBUF num(%d) [%d] [%d] [%d] ", bufsOnHal,indexToQueue[0],indexToQueue[1],indexToQueue[2] );
            return;
        }



        while (m_bayerBufStatus[index] != BAYER_ON_HAL_EMPTY) // TODO : use signal
            usleep(5000);

        // TODO : instead of re-using 'index', query reqManager about free entry

        if (m_requestManager->MarkProcessingRequest(&(m_camera_info.sensor.buffer[index]))!=NO_ERROR) {
            ALOGV("DEBUG(%s) inserting bubble to index(%d)", __FUNCTION__, index);
      		shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);
            memcpy(shot_ext, &(m_camera_info.dummy_shot), sizeof(camera2_shot_ext));
        }
        m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 33*1000*1000;
/*
		shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);
		shot_ext->request_sensor = m_camera_info.current_shot.request_sensor;
		shot_ext->request_scc = m_camera_info.current_shot.request_scc;
		shot_ext->request_scp = m_camera_info.current_shot.request_scp;
		shot_ext->shot.magicNumber = m_camera_info.current_shot.shot.magicNumber;
		memcpy(&shot_ext->shot.ctl, &m_camera_info.current_shot.shot.ctl,
				sizeof(struct camera2_ctl));
*/
        // FOR DEBUG
        //shot_ext->shot.ctl.request.id = m_camera_info.sensor_frame_count;

        //ALOGV("### isp QBUF start index(%d)", index);
        //cam_int_qbuf(&(m_camera_info.isp), index);
        //ALOGV("### isp QBUF done and calling DQBUF");
        //index = cam_int_dqbuf(&(m_camera_info.isp));
        //ALOGV("### isp DQBUF done index(%d)", index);

		{
//			m_camera_info.current_shot.shot.ctl.sensor.frameDuration = 33*1000*1000;
			//m_camera_info.current_shot.shot.ctl.sensor.frameDuration = 66*1000*1000;
//			m_camera_info.current_shot.request_scp = 1;
			//m_camera_info.sensor_frame_count++;
		}
/*		memcpy(&shot_ext->shot.ctl.sensor,
    		&m_camera_info.current_shot.shot.ctl.sensor,
    		sizeof(struct camera2_sensor_ctl));*/
        ALOGV("### Sensor QBUF start index(%d)", index);
        cam_int_qbuf(&(m_camera_info.sensor), index);
        m_bayerBufStatus[index] = BAYER_ON_SENSOR;
        ALOGV("### Sensor QBUF done");


        selfThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
#endif
        }
    return;
}


void ExynosCameraHWInterface2::m_ispThreadInitialize(SignalDrivenThread * self)
{
    ALOGV("DEBUG(%s): ", __FUNCTION__ );
    IspThread * selfThread = ((IspThread*)self);
    char node_name[30];
    int fd = 0;
    int i =0, j=0;


    while (!m_initFlag1) //temp
        usleep(100000);

    /*isp init*/
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 41);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open isp video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): isp video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.isp.fd = fd;

    m_camera_info.isp.width = m_camera_info.sensor.width;
    m_camera_info.isp.height = m_camera_info.sensor.height;
    m_camera_info.isp.format = m_camera_info.sensor.format;
    m_camera_info.isp.planes = m_camera_info.sensor.planes;
    m_camera_info.isp.buffers = m_camera_info.sensor.buffers;
    m_camera_info.isp.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    m_camera_info.isp.memory = V4L2_MEMORY_DMABUF;
    //m_camera_info.isp.ionClient = m_ionCameraClient;
/*
    for(i = 0; i < m_camera_info.isp.buffers; i++){
        initCameraMemory(&m_camera_info.isp.buffer[i], m_camera_info.isp.planes);
        m_camera_info.isp.buffer[i].size.extS[0] = m_camera_info.isp.width*m_camera_info.isp.height*2;
        allocCameraMemory(m_camera_info.isp.ionClient, &m_camera_info.isp.buffer[i], m_camera_info.isp.planes);
    };
*/

    for(i = 0; i < m_camera_info.isp.buffers; i++){
        initCameraMemory(&m_camera_info.isp.buffer[i], m_camera_info.isp.planes);
        m_camera_info.isp.buffer[i].size.extS[0]    = m_camera_info.sensor.buffer[i].size.extS[0];
        m_camera_info.isp.buffer[i].size.extS[1]    = m_camera_info.sensor.buffer[i].size.extS[1];
        m_camera_info.isp.buffer[i].fd.extFd[0]     = m_camera_info.sensor.buffer[i].fd.extFd[0];
        m_camera_info.isp.buffer[i].fd.extFd[1]     = m_camera_info.sensor.buffer[i].fd.extFd[1];
        m_camera_info.isp.buffer[i].virt.extP[0]    = m_camera_info.sensor.buffer[i].virt.extP[0];
        m_camera_info.isp.buffer[i].virt.extP[1]    = m_camera_info.sensor.buffer[i].virt.extP[1];
    };

    ALOGV("DEBUG(%s): isp mem alloc done",  __FUNCTION__);
    cam_int_s_input(&(m_camera_info.sensor), m_camera_info.sensor_id);
    ALOGV("DEBUG(%s): sensor s_input done",  __FUNCTION__);

    if (cam_int_s_fmt(&(m_camera_info.sensor))< 0) {
        ALOGE("ERR(%s): sensor s_fmt fail",  __FUNCTION__);
    }
    ALOGV("DEBUG(%s): sensor s_fmt done",  __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.sensor));
    ALOGV("DEBUG(%s): sensor reqbuf done",  __FUNCTION__);
    for (i = 0; i < m_camera_info.sensor.buffers; i++) {
        ALOGV("DEBUG(%s): sensor initial QBUF [%d]",  __FUNCTION__, i);
        memcpy( m_camera_info.sensor.buffer[i].virt.extP[1], &(m_camera_info.dummy_shot),
                sizeof(struct camera2_shot_ext));
        m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 33*1000*1000; // apply from frame #1

        cam_int_qbuf(&(m_camera_info.sensor), i);
        m_bayerBufStatus[i] = BAYER_ON_SENSOR;
        RegisterBayerQueueList(i, -1);
    }
    cam_int_streamon(&(m_camera_info.sensor));

//    m_camera_info.sensor.currentBufferIndex = 0;

    cam_int_s_input(&(m_camera_info.isp), m_camera_info.sensor_id);
    cam_int_s_fmt(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling reqbuf", __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling querybuf", __FUNCTION__);

    for (i = 0; i < m_camera_info.isp.buffers; i++) {
        ALOGV("DEBUG(%s): isp initial QBUF [%d]",  __FUNCTION__, i);
        cam_int_qbuf(&(m_camera_info.isp), i);
    }
    cam_int_streamon(&(m_camera_info.isp));

    for (i = 0; i < m_camera_info.isp.buffers; i++) {
        ALOGV("DEBUG(%s): isp initial DQBUF [%d]",  __FUNCTION__, i);
        cam_int_dqbuf(&(m_camera_info.isp));
    }

/*capture init*/
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 42);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open capture video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): capture video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.capture.fd = fd;
    m_camera_info.capture.width = 2560;
    m_camera_info.capture.height = 1920;
    m_camera_info.capture.format = V4L2_PIX_FMT_YUYV;
    m_camera_info.capture.planes = 1;
    m_camera_info.capture.buffers = 8;
    m_camera_info.capture.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_camera_info.capture.memory = V4L2_MEMORY_DMABUF;
    m_camera_info.capture.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.capture.buffers; i++){
        initCameraMemory(&m_camera_info.capture.buffer[i], m_camera_info.capture.planes);
        m_camera_info.capture.buffer[i].size.extS[0] = m_camera_info.capture.width*m_camera_info.capture.height*2;
        allocCameraMemory(m_camera_info.capture.ionClient, &m_camera_info.capture.buffer[i], m_camera_info.capture.planes);
    }

    cam_int_s_input(&(m_camera_info.capture), m_camera_info.sensor_id);
    cam_int_s_fmt(&(m_camera_info.capture));
    ALOGV("DEBUG(%s): capture calling reqbuf", __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.capture));
    ALOGV("DEBUG(%s): capture calling querybuf", __FUNCTION__);

    for (i = 0; i < m_camera_info.capture.buffers; i++) {
        ALOGV("DEBUG(%s): capture initial QBUF [%d]",  __FUNCTION__, i);
        cam_int_qbuf(&(m_camera_info.capture), i);
    }
    cam_int_streamon(&(m_camera_info.capture));

    m_initFlag2 = true;
    ALOGV("DEBUG(%s): END of IspThreadInitialize ", __FUNCTION__);
    return;
}


void ExynosCameraHWInterface2::m_ispThreadFunc(SignalDrivenThread * self)
{
    uint32_t        currentSignal = self->GetProcessingSignal();
    IspThread *  selfThread      = ((IspThread*)self);
    int index;
    status_t res;
    ALOGV("DEBUG(%s): m_ispThreadFunc (%x)", __FUNCTION__, currentSignal);

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE", __FUNCTION__);

        ALOGV("DEBUG(%s): calling capture streamoff", __FUNCTION__);
        cam_int_streamoff(&(m_camera_info.capture));
        ALOGV("DEBUG(%s): calling capture streamoff done", __FUNCTION__);
        /*
        ALOGV("DEBUG(%s): calling capture s_ctrl done", __FUNCTION__);
        m_camera_info.capture.buffers = 0;
        cam_int_reqbufs(&(m_camera_info.capture));
        ALOGV("DEBUG(%s): calling capture reqbuf 0 done", __FUNCTION__);
*/
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE DONE", __FUNCTION__);
        selfThread->SetSignal(SIGNAL_THREAD_TERMINATE);
        return;
    }
    if (currentSignal & SIGNAL_ISP_START_BAYER_INPUT)
    {
        struct camera2_shot_ext *shot_ext =
            (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[m_ispInputIndex].virt.extP[1]);
/*
//        int targetStreamIndex = 0;
        struct camera2_shot_ext *shot_ext;

        shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[m_ispInputIndex].virt.extP[1]);
		shot_ext->request_sensor = m_camera_info.current_shot.request_sensor;
		shot_ext->request_scc = m_camera_info.current_shot.request_scc;
		shot_ext->request_scp = m_camera_info.current_shot.request_scp;
		shot_ext->shot.magicNumber = m_camera_info.current_shot.shot.magicNumber;
		memcpy(&shot_ext->shot.ctl, &m_camera_info.current_shot.shot.ctl,
				sizeof(struct camera2_ctl));
        ALOGV("DEBUG(%s): IspThread processing SIGNAL_ISP_START_BAYER_INPUT id-dm(%d) id-ctl(%d) frameCnt-dm(%d) scp(%d) scc(%d) magic(%x)",
                __FUNCTION__, shot_ext->shot.dm.request.id, shot_ext->shot.ctl.request.id, shot_ext->shot.dm.sensor.frameCount,
                 shot_ext->request_scp, shot_ext->request_scc, shot_ext->shot.magicNumber);
        ALOGV("DEBUG(%s): m_numExpRemainingOutScp = %d  m_numExpRemainingOutScc = %d", __FUNCTION__, m_numExpRemainingOutScp, m_numExpRemainingOutScc);
 	*/
 	    ALOGV("DEBUG(%s): IspThread processing SIGNAL_ISP_START_BAYER_INPUT", __FUNCTION__);
        m_ispProcessingIndex = m_ispInputIndex;
        m_ispThreadProcessingReq = m_processingRequest;
        m_ispInputIndex = -1;
        ALOGV("### isp QBUF start index(%d) => for request(%d)", m_ispProcessingIndex, m_ispThreadProcessingReq);

        if (m_ispThreadProcessingReq != -1) {
            // HACK : re-write request info here
            ALOGV("### Re-writing output stream info");
            m_requestManager->UpdateOutputStreamInfo(shot_ext, m_ispThreadProcessingReq);
	        DumpInfoWithShot(shot_ext);
        }
        if (m_scp_flushing) {
            shot_ext->request_scp = 1;
        }
        cam_int_qbuf(&(m_camera_info.isp), m_ispProcessingIndex);
        m_bayerBufStatus[m_ispProcessingIndex] = BAYER_ON_ISP;
        ALOGV("### isp QBUF done and calling DQBUF");
        if (m_ispThreadProcessingReq != -1) // bubble
        {
            if (shot_ext->request_scc) {
                m_numExpRemainingOutScc++;
                m_streamThreads[1]->SetSignal(SIGNAL_STREAM_DATA_COMING);
            }
            if (shot_ext->request_scp) {
                m_numExpRemainingOutScp++;
                m_streamThreads[0]->SetSignal(SIGNAL_STREAM_DATA_COMING);
            }
            m_lastTimeStamp = systemTime();

        }
/*
#if 1 // for test

        if (shot_ext->request_scp) {
            m_numExpRemainingOutScp++;
            m_streamThreads[0]->SetSignal(SIGNAL_STREAM_DATA_COMING);
        }
        if (shot_ext->request_scc) {
            m_numExpRemainingOutScc++;
            m_streamThreads[1]->SetSignal(SIGNAL_STREAM_DATA_COMING);
        }

#else
        if (currentEntry) {
            for (int i = 0; i < currentEntry->output_stream_count; i++) {
                targetStreamIndex = currentEntry->internal_shot.ctl.request.outputStreams[i];
                    // TODO : match with actual stream index;
                 ALOGV("### outputstream(%d) sending data signal to stream [%d]", i, targetStreamIndex);
                m_streamThreads[targetStreamIndex]->SetSignal(SIGNAL_STREAM_DATA_COMING);
            }
        }
#endif

shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);
        ALOGV("DEBUG(%s): information of DQed buffer id-dm(%d) id-ctl(%d) frameCnt-dm(%d) scp(%d) magic(%x)",
                __FUNCTION__, shot_ext->shot.dm.request.id, shot_ext->shot.ctl.request.id, shot_ext->shot.dm.sensor.frameCount,
                    shot_ext->request_scp, shot_ext->shot.magicNumber);
*/
        index = cam_int_dqbuf(&(m_camera_info.isp));
        ALOGD("### isp DQBUF done index(%d) => for request(%d)", index, m_ispThreadProcessingReq);
        if (m_ispThreadProcessingReq != -1) { // bubble
            //DumpFrameinfoWithBufIndex(index);
            shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);
	        DumpInfoWithShot(shot_ext);
            m_requestManager->ApplyDynamicMetadata(m_ispThreadProcessingReq);
        }
        m_bayerBufStatus[index] = BAYER_ON_HAL_EMPTY;
        RegisterBayerDequeueList(index);
    }
    return;
}

void ExynosCameraHWInterface2::m_streamThreadInitialize(SignalDrivenThread * self)
{
    StreamThread *          selfThread      = ((StreamThread*)self);
    ALOGV("DEBUG(%s): ", __FUNCTION__ );
    memset(&(selfThread->m_parameters), 0, sizeof(stream_parameters_t));
    selfThread->m_isBufferInit = false;

    return;
}


void ExynosCameraHWInterface2::m_streamThreadFunc(SignalDrivenThread * self)
{
    uint32_t                currentSignal   = self->GetProcessingSignal();
    StreamThread *          selfThread      = ((StreamThread*)self);
    stream_parameters_t     *selfStreamParms =  &(selfThread->m_parameters);
    node_info_t             *currentNode    = &(selfStreamParms->node);

    ALOGV("DEBUG(%s): m_streamThreadFunc[%d] (%x)", __FUNCTION__, selfThread->m_index, currentSignal);

    if (currentSignal & SIGNAL_STREAM_CHANGE_PARAMETER) {
        ALOGV("DEBUG(%s): processing SIGNAL_STREAM_CHANGE_PARAMETER", __FUNCTION__);
        selfThread->applyChange();
        if (selfStreamParms->streamType==1) {
            m_resizeBuf.size.extS[0] = ALIGN(selfStreamParms->outputWidth, 16) * ALIGN(selfStreamParms->outputHeight, 16) * 2;
            m_resizeBuf.size.extS[1] = 0;
            m_resizeBuf.size.extS[2] = 0;

            if (allocCameraMemory(selfStreamParms->ionClient, &m_resizeBuf, 1) == -1) {
                ALOGE("ERR(%s): Failed to allocate resize buf", __FUNCTION__);
            }
        }
        ALOGV("DEBUG(%s): processing SIGNAL_STREAM_CHANGE_PARAMETER DONE", __FUNCTION__);
    }

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        int i, index = -1, cnt_to_dq=0;
        status_t res;
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE", __FUNCTION__);



        if (selfThread->m_isBufferInit) {
            for ( i=0 ; i < selfStreamParms->numSvcBuffers; i++) {
                ALOGV("DEBUG(%s): checking buffer index[%d] - status(%d)",
                    __FUNCTION__, i, selfStreamParms->svcBufStatus[i]);
                if (selfStreamParms->svcBufStatus[i] ==ON_DRIVER) cnt_to_dq++;
            }
            m_scp_flushing = true;
            for ( i=0 ; i < cnt_to_dq ; i++) {
                ALOGV("@@@@@@ dq start");
                index = cam_int_dqbuf(&(selfStreamParms->node));
                ALOGV("@@@@@@ dq done, index(%d)", index);
                if (index >=0 && index < selfStreamParms->numSvcBuffers) {
                    selfStreamParms->svcBufStatus[index] = ON_HAL;
                }
            }
            m_scp_flushing = false;
            ALOGV("DEBUG(%s): calling stream(%d) streamoff (fd:%d)", __FUNCTION__,
            selfThread->m_index, selfStreamParms->fd);
            cam_int_streamoff(&(selfStreamParms->node));
            ALOGV("DEBUG(%s): calling stream(%d) streamoff done", __FUNCTION__, selfThread->m_index);

            for ( i=0 ; i < selfStreamParms->numSvcBuffers; i++) {
                ALOGV("DEBUG(%s): releasing buffer index[%d] - status(%d)",
                    __FUNCTION__, i, selfStreamParms->svcBufStatus[i]);

                switch (selfStreamParms->svcBufStatus[i]) {

                case ON_DRIVER:
                    ALOGV("@@@@@@ this should not happen");
                case ON_HAL:
                    res = selfStreamParms->streamOps->cancel_buffer(selfStreamParms->streamOps,
                            &(selfStreamParms->svcBufHandle[i]));
                    if (res != NO_ERROR ) {
                        ALOGE("ERR(%s): unable to cancel buffer : %d",__FUNCTION__ , res);
                         // TODO : verify after service is ready
                         // return;
                    }
                    break;
                case ON_SERVICE:
                default:
                    break;

                }
            }
        }
        if (selfStreamParms->streamType==1) {
            if (m_resizeBuf.size.s != 0) {
                freeCameraMemory(&m_resizeBuf, 1);
            }
        }

        selfThread->m_index = 255;
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE DONE", __FUNCTION__);
        selfThread->SetSignal(SIGNAL_THREAD_TERMINATE);
        return;
    }

    if (currentSignal & SIGNAL_STREAM_DATA_COMING) {
        buffer_handle_t * buf = NULL;
        status_t res;
        void *virtAddr[3];
        int i, j;
        int index;
        ALOGV("DEBUG(%s): stream(%d) processing SIGNAL_STREAM_DATA_COMING",
            __FUNCTION__,selfThread->m_index);
        if (!(selfThread->m_isBufferInit)) {
            for ( i=0 ; i < selfStreamParms->numSvcBuffers; i++) {
                res = selfStreamParms->streamOps->dequeue_buffer(selfStreamParms->streamOps, &buf);
                if (res != NO_ERROR || buf == NULL) {
                    ALOGE("ERR(%s): Init: unable to dequeue buffer : %d",__FUNCTION__ , res);
                    return;
                }
                ALOGV("DEBUG(%s): got buf(%x) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
                   ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

                if (m_grallocHal->lock(m_grallocHal, *buf,
                           selfStreamParms->usage,
                           0, 0, selfStreamParms->outputWidth, selfStreamParms->outputHeight, virtAddr) != 0) {
                    ALOGE("ERR(%s): could not obtain gralloc buffer", __FUNCTION__);
                    return;
                }
                ALOGV("DEBUG(%s): locked img buf plane0(%x) plane1(%x) plane2(%x)",
                __FUNCTION__, (unsigned int)virtAddr[0], (unsigned int)virtAddr[1], (unsigned int)virtAddr[2]);

                index = selfThread->findBufferIndex(virtAddr[0]);
                if (index == -1) {
                    ALOGE("ERR(%s): could not find buffer index", __FUNCTION__);
                }
                else {
                    ALOGV("DEBUG(%s): found buffer index[%d] - status(%d)",
                        __FUNCTION__, index, selfStreamParms->svcBufStatus[index]);
                    if (selfStreamParms->svcBufStatus[index]== REQUIRES_DQ_FROM_SVC)
                        selfStreamParms->svcBufStatus[index] = ON_DRIVER;
                    else if (selfStreamParms->svcBufStatus[index]== ON_SERVICE)
                        selfStreamParms->svcBufStatus[index] = ON_HAL;
                    else {
                        ALOGD("DBG(%s): buffer status abnormal (%d) "
                            , __FUNCTION__, selfStreamParms->svcBufStatus[index]);
                    }
                    if (*buf != selfStreamParms->svcBufHandle[index])
                        ALOGD("DBG(%s): different buf_handle index ", __FUNCTION__);
                    else
                        ALOGV("DEBUG(%s): same buf_handle index", __FUNCTION__);
                }
                m_svcBufIndex = 0;
            }
            selfThread->m_isBufferInit = true;
        }


        if (selfStreamParms->streamType==0) {
            ALOGV("DEBUG(%s): stream(%d) type(%d) DQBUF START ",__FUNCTION__,
                selfThread->m_index, selfStreamParms->streamType);
/*
            for ( i=0 ; i < selfStreamParms->numSvcBuffers; i++) {
                ALOGV("DEBUG(%s): STREAM BUF status index[%d] - status(%d)",
                    __FUNCTION__, i, selfStreamParms->svcBufStatus[i]);
            }
*/
            index = cam_int_dqbuf(&(selfStreamParms->node));
            ALOGV("DEBUG(%s): stream(%d) type(%d) DQBUF done index(%d)",__FUNCTION__,
                selfThread->m_index, selfStreamParms->streamType, index);

            m_numExpRemainingOutScp--;

            if (selfStreamParms->svcBufStatus[index] !=  ON_DRIVER)
                ALOGD("DBG(%s): DQed buffer status abnormal (%d) ",
                       __FUNCTION__, selfStreamParms->svcBufStatus[index]);
            selfStreamParms->svcBufStatus[index] = ON_HAL;
            res = selfStreamParms->streamOps->enqueue_buffer(selfStreamParms->streamOps,
                    m_requestManager->GetTimestamp(m_ispThreadProcessingReq), &(selfStreamParms->svcBufHandle[index]));
            ALOGV("DEBUG(%s): stream(%d) enqueue_buffer to svc done res(%d)", __FUNCTION__, selfThread->m_index, res);
            if (res == 0) {
                selfStreamParms->svcBufStatus[index] = ON_SERVICE;
            }
            else {
                selfStreamParms->svcBufStatus[index] = ON_HAL;
            }
            m_requestManager->NotifyStreamOutput(m_ispThreadProcessingReq, selfThread->m_index);
        }
        else if (selfStreamParms->streamType==1) {
            ALOGV("DEBUG(%s): stream(%d) type(%d) DQBUF START ",__FUNCTION__,
                selfThread->m_index, selfStreamParms->streamType);
            index = cam_int_dqbuf(&(selfStreamParms->node));
            ALOGV("DEBUG(%s): stream(%d) type(%d) DQBUF done index(%d)",__FUNCTION__,
                selfThread->m_index, selfStreamParms->streamType, index);

            m_numExpRemainingOutScc--;
            m_jpegEncodingRequestIndex = m_ispThreadProcessingReq;

            bool ret = false;
            int pictureW, pictureH, pictureFramesize = 0;
            int pictureFormat;
            int cropX, cropY, cropW, cropH = 0;


            ExynosBuffer jpegBuf;

            ExynosRect   m_orgPictureRect;

            m_orgPictureRect.w = selfStreamParms->outputWidth;
            m_orgPictureRect.h = selfStreamParms->outputHeight;

            ExynosBuffer* m_pictureBuf = &(m_camera_info.capture.buffer[index]);

            pictureW = 2560;
            pictureH = 1920;

            pictureFormat = V4L2_PIX_FMT_YUYV;
            pictureFramesize = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat), pictureW, pictureH);

            // resize from pictureBuf(max size) to rawHeap(user's set size)
            if (m_exynosPictureCSC) {
                m_getRatioSize(pictureW, pictureH,
                               m_orgPictureRect.w, m_orgPictureRect.h,
                               &cropX, &cropY,
                               &cropW, &cropH,
                               0); //m_secCamera->getZoom());

                ALOGV("DEBUG(%s):cropX = %d, cropY = %d, cropW = %d, cropH = %d",
                      __FUNCTION__, cropX, cropY, cropW, cropH);

                csc_set_src_format(m_exynosPictureCSC,
                                   ALIGN(pictureW, 16), ALIGN(pictureH, 16),
                                   cropX, cropY, cropW, cropH,
                                   V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat),
                                   0);

                csc_set_dst_format(m_exynosPictureCSC,
                                   m_orgPictureRect.w, m_orgPictureRect.h,
                                   0, 0, m_orgPictureRect.w, m_orgPictureRect.h,
                                   V4L2_PIX_2_HAL_PIXEL_FORMAT(V4L2_PIX_FMT_NV16),
                                   0);
                csc_set_src_buffer(m_exynosPictureCSC,
                                   (void **)&m_pictureBuf->fd.fd);

                csc_set_dst_buffer(m_exynosPictureCSC,
                                   (void **)&m_resizeBuf.fd.fd);
                for (int i=0 ; i < 3 ; i++)
                    ALOGV("DEBUG(%s): m_resizeBuf.virt.extP[%d]=%x m_resizeBuf.size.extS[%d]=%d",
                        __FUNCTION__, i, m_resizeBuf.fd.extFd[i], i, m_resizeBuf.size.extS[i]);

                if (csc_convert(m_exynosPictureCSC) != 0)
                    ALOGE("ERR(%s): csc_convert() fail", __FUNCTION__);

                for (int i=0 ; i < 3 ; i++)
                    ALOGV("DEBUG(%s): m_resizeBuf.virt.extP[%d]=%x m_resizeBuf.size.extS[%d]=%d",
                        __FUNCTION__, i, m_resizeBuf.fd.extFd[i], i, m_resizeBuf.size.extS[i]);
            }
            else {
                ALOGE("ERR(%s): m_exynosPictureCSC == NULL", __FUNCTION__);
            }

            m_getAlignedYUVSize(V4L2_PIX_FMT_NV16, m_orgPictureRect.w, m_orgPictureRect.h, &m_resizeBuf);

            for (int i=0 ; i < 3 ; i++) {
                ALOGV("DEBUG(%s): m_resizeBuf.virt.extP[%d]=%x m_resizeBuf.size.extS[%d]=%d",
                            __FUNCTION__, i, m_resizeBuf.fd.extFd[i], i, m_resizeBuf.size.extS[i]);
            }

            for (int i = 1; i < 3; i++) {
                if (m_resizeBuf.size.extS[i] != 0)
                    m_resizeBuf.fd.extFd[i] = m_resizeBuf.fd.extFd[i-1] + m_resizeBuf.size.extS[i-1];

                ALOGV("(%s): m_resizeBuf.size.extS[%d] = %d", __FUNCTION__, i, m_resizeBuf.size.extS[i]);
            }


            ExynosRect jpegRect;
            bool found = false;
            jpegRect.w = m_orgPictureRect.w;
            jpegRect.h = m_orgPictureRect.h;
            jpegRect.colorFormat = V4L2_PIX_FMT_NV16;

            jpegBuf.size.extS[0] = 5*1024*1024;
            jpegBuf.size.extS[1] = 0;
            jpegBuf.size.extS[2] = 0;

            allocCameraMemory(currentNode->ionClient, &jpegBuf, 1);

            ALOGV("DEBUG(%s): jpegBuf.size.s = %d , jpegBuf.virt.p = %x", __FUNCTION__,
                jpegBuf.size.s, jpegBuf.virt.p);


            if (yuv2Jpeg(&m_resizeBuf, &jpegBuf, &jpegRect) == false)
                ALOGE("ERR(%s):yuv2Jpeg() fail", __FUNCTION__);
            cam_int_qbuf(&(selfStreamParms->node), index);
            ALOGV("DEBUG(%s): stream(%d) type(%d) QBUF DONE ",__FUNCTION__,
                selfThread->m_index, selfStreamParms->streamType);

            for (int i = 0; i < selfStreamParms->numSvcBuffers ; i++) {
                if (selfStreamParms->svcBufStatus[m_svcBufIndex] == ON_HAL) {
                    found = true;
                    break;
                }
                m_svcBufIndex++;
                if (m_svcBufIndex >= selfStreamParms->numSvcBuffers) m_svcBufIndex = 0;
            }
            if (!found) {
                ALOGE("ERR(%s): NO free SVC buffer for JPEG", __FUNCTION__);
            }
            else {
                memcpy(selfStreamParms->svcBuffers[m_svcBufIndex].virt.extP[0], jpegBuf.virt.extP[0], 5*1024*1024);

                res = selfStreamParms->streamOps->enqueue_buffer(selfStreamParms->streamOps,
                        m_requestManager->GetTimestamp(m_jpegEncodingRequestIndex), &(selfStreamParms->svcBufHandle[m_svcBufIndex]));

                freeCameraMemory(&jpegBuf, 1);
                ALOGV("DEBUG(%s): stream(%d) enqueue_buffer index(%d) to svc done res(%d)",
                        __FUNCTION__, selfThread->m_index, m_svcBufIndex, res);
                if (res == 0) {
                    selfStreamParms->svcBufStatus[m_svcBufIndex] = ON_SERVICE;
                }
                else {
                    selfStreamParms->svcBufStatus[m_svcBufIndex] = ON_HAL;
                }
                m_requestManager->NotifyStreamOutput(m_jpegEncodingRequestIndex, selfThread->m_index);
            }

        }
        while(1) {
            res = selfStreamParms->streamOps->dequeue_buffer(selfStreamParms->streamOps, &buf);
            if (res != NO_ERROR || buf == NULL) {
                ALOGV("DEBUG(%s): stream(%d) dequeue_buffer fail res(%d)",__FUNCTION__ , selfThread->m_index,  res);
                break;
            }

            ALOGV("DEBUG(%s): got buf(%x) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
               ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

            if (m_grallocHal->lock(m_grallocHal, *buf,
                       selfStreamParms->usage,
                       0, 0, selfStreamParms->outputWidth, selfStreamParms->outputHeight, virtAddr) != 0) {

                ALOGE("ERR(%s):could not obtain gralloc buffer", __FUNCTION__);
            }
            ALOGV("DEBUG(%s): locked img buf plane0(%x) plane1(%x) plane2(%x)", __FUNCTION__,
                (unsigned int)virtAddr[0], (unsigned int)virtAddr[1], (unsigned int)virtAddr[2]);

            index = selfThread->findBufferIndex(virtAddr[0]);
            if (index == -1) {
                ALOGD("DBG(%s): could not find buffer index", __FUNCTION__);
            }
            else {
                ALOGV("DEBUG(%s): found buffer index[%d]", __FUNCTION__, index);

                if (selfStreamParms->svcBufStatus[index] != ON_SERVICE)
                    ALOGD("DBG(%s): dequeued buf status abnormal (%d)", __FUNCTION__, selfStreamParms->svcBufStatus[index]);
                else {
                    selfStreamParms->svcBufStatus[index] = ON_HAL;
                    if (index < selfStreamParms->numHwBuffers) {

                        uint32_t    plane_index = 0;
                        ExynosBuffer*  currentBuf = &(selfStreamParms->svcBuffers[index]);
                        const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);
                        struct v4l2_buffer v4l2_buf;
                        struct v4l2_plane  planes[VIDEO_MAX_PLANES];

                        v4l2_buf.m.planes   = planes;
                        v4l2_buf.type       = currentNode->type;
                        v4l2_buf.memory     = currentNode->memory;
                        v4l2_buf.index      = index;
                        v4l2_buf.length     = currentNode->planes;

                        v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                        v4l2_buf.m.planes[1].m.fd = priv_handle->u_fd;
                        v4l2_buf.m.planes[2].m.fd = priv_handle->v_fd;
                        for (plane_index=0 ; plane_index < v4l2_buf.length ; plane_index++) {
                            v4l2_buf.m.planes[plane_index].length  = currentBuf->size.extS[plane_index];
                            ALOGV("DEBUG(%s): plane(%d): fd(%d) addr(%x), length(%d)",
                                 __FUNCTION__, plane_index, v4l2_buf.m.planes[plane_index].m.fd,
                                (unsigned long)(virtAddr[plane_index]), v4l2_buf.m.planes[plane_index].length);
                        }

                        if (selfStreamParms->streamType == 0) {
                            if (exynos_v4l2_qbuf(currentNode->fd, &v4l2_buf) < 0) {
                                ALOGE("ERR(%s): stream id(%d) exynos_v4l2_qbuf() fail",
                                    __FUNCTION__, selfThread->m_index);
                                return;
                            }
                            selfStreamParms->svcBufStatus[index] = ON_DRIVER;
                            ALOGV("DEBUG(%s): stream id(%d) type0 QBUF done index(%d)",
                                __FUNCTION__, selfThread->m_index, index);
                        }
                        else if (selfStreamParms->streamType == 1) {
                            selfStreamParms->svcBufStatus[index]  = ON_HAL;
                            ALOGV("DEBUG(%s): stream id(%d) type1 DQBUF done index(%d)",
                                __FUNCTION__, selfThread->m_index, index);
                        }

                    }
                }
            }
        }
        ALOGV("DEBUG(%s): stream(%d) processing SIGNAL_STREAM_DATA_COMING DONE",
            __FUNCTION__,selfThread->m_index);
    }
    return;
}

bool ExynosCameraHWInterface2::yuv2Jpeg(ExynosBuffer *yuvBuf,
                            ExynosBuffer *jpegBuf,
                            ExynosRect *rect)
{
    unsigned char *addr;

    ExynosJpegEncoderForCamera jpegEnc;
    bool ret = false;
    int res = 0;

    unsigned int *yuvSize = yuvBuf->size.extS;

    if (jpegEnc.create()) {
        ALOGE("ERR(%s):jpegEnc.create() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setQuality(100)) {
        ALOGE("ERR(%s):jpegEnc.setQuality() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setSize(rect->w, rect->h)) {
        ALOGE("ERR(%s):jpegEnc.setSize() fail", __func__);
        goto jpeg_encode_done;
    }
    ALOGV("%s : width = %d , height = %d\n", __FUNCTION__, rect->w, rect->h);

    if (jpegEnc.setColorFormat(rect->colorFormat)) {
        ALOGE("ERR(%s):jpegEnc.setColorFormat() fail", __func__);
        goto jpeg_encode_done;
    }
    ALOGV("%s : color = %s\n", __FUNCTION__, &(rect->colorFormat));

    if (jpegEnc.setJpegFormat(V4L2_PIX_FMT_JPEG_422)) {
        ALOGE("ERR(%s):jpegEnc.setJpegFormat() fail", __func__);
        goto jpeg_encode_done;
    }
#if 0
    if (m_curCameraInfo->thumbnailW != 0 && m_curCameraInfo->thumbnailH != 0) {
        int thumbW = 0, thumbH = 0;
        mExifInfo.enableThumb = true;
        if (rect->w < 320 || rect->h < 240) {
            thumbW = 160;
            thumbH = 120;
        } else {
            thumbW = m_curCameraInfo->thumbnailW;
            thumbH = m_curCameraInfo->thumbnailH;
        }
        if (jpegEnc.setThumbnailSize(thumbW, thumbH)) {
            LOGE("ERR(%s):jpegEnc.setThumbnailSize(%d, %d) fail", __func__, thumbW, thumbH);
            goto jpeg_encode_done;
        }

        if (0 < m_jpegThumbnailQuality && m_jpegThumbnailQuality <= 100) {
            if (jpegEnc.setThumbnailQuality(m_jpegThumbnailQuality)) {
                LOGE("ERR(%s):jpegEnc.setThumbnailQuality(%d) fail", __func__, m_jpegThumbnailQuality);
                goto jpeg_encode_done;
            }
        }

        m_setExifChangedAttribute(&mExifInfo, rect);
    } else
#endif
    {
        mExifInfo.enableThumb = false;
    }
    ALOGV("DEBUG(%s):calling jpegEnc.setInBuf() yuvSize(%d)", __func__, *yuvSize);
    for (int i=0 ; i < 3 ; i++)
            ALOGV("DEBUG(%s):calling jpegEnc.setInBuf() virt.extP[%d]=%x extS[%d]=%d",
                __FUNCTION__, i, yuvBuf->fd.extFd[i], i, yuvBuf->size.extS[i]);
    if (jpegEnc.setInBuf((int *)&(yuvBuf->fd.fd), (int *)yuvSize)) {
        ALOGE("ERR(%s):jpegEnc.setInBuf() fail", __func__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setOutBuf(jpegBuf->fd.fd, jpegBuf->size.extS[0] + jpegBuf->size.extS[1] + jpegBuf->size.extS[2])) {
        ALOGE("ERR(%s):jpegEnc.setOutBuf() fail", __func__);
        goto jpeg_encode_done;
    }
    for (int i=0 ; i < 3 ; i++)
        ALOGV("DEBUG(%s): jpegBuf->virt.extP[%d]=%x   jpegBuf->size.extS[%d]=%d",
                __func__, i, jpegBuf->fd.extFd[i], i, jpegBuf->size.extS[i]);
    memset(jpegBuf->virt.p,0,jpegBuf->size.extS[0] + jpegBuf->size.extS[1] + jpegBuf->size.extS[2]);

    if (jpegEnc.updateConfig()) {
        ALOGE("ERR(%s):jpegEnc.updateConfig() fail", __func__);
        goto jpeg_encode_done;
    }

    if (res = jpegEnc.encode((int *)&jpegBuf->size.s, NULL)) {
        ALOGE("ERR(%s):jpegEnc.encode() fail ret(%d)", __func__, res);
        goto jpeg_encode_done;
    }

    ret = true;

jpeg_encode_done:

    if (jpegEnc.flagCreate() == true)
        jpegEnc.destroy();

    return ret;
}


ExynosCameraHWInterface2::MainThread::~MainThread()
{
    ALOGV("DEBUG(%s):", __func__);
}

void ExynosCameraHWInterface2::MainThread::release()
{
    ALOGV("DEBUG(%s):", __func__);

    SetSignal(SIGNAL_THREAD_RELEASE);

    // TODO : return synchronously (after releasing asynchronously)
    usleep(400000);
    //while (m_index != 255)  // temp.. To make smarter..
    //    usleep(200000);
    SetSignal(SIGNAL_THREAD_TERMINATE);
    ALOGV("DEBUG(%s): DONE", __func__);
}

ExynosCameraHWInterface2::SensorThread::~SensorThread()
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::SensorThread::release()
{
    ALOGV("DEBUG(%s):", __func__);

    SetSignal(SIGNAL_THREAD_RELEASE);

    // TODO : return synchronously (after releasing asynchronously)
    usleep(400000);
    //while (m_index != 255)  // temp.. To make smarter..
    //    usleep(200000);
    SetSignal(SIGNAL_THREAD_TERMINATE);
    ALOGV("DEBUG(%s): DONE", __func__);
}

ExynosCameraHWInterface2::IspThread::~IspThread()
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::IspThread::release()
{
    ALOGV("DEBUG(%s):", __func__);

    SetSignal(SIGNAL_THREAD_RELEASE);

    // TODO : return synchronously (after releasing asynchronously)
    usleep(400000);
    //while (m_index != 255)  // temp.. To make smarter..
    //    usleep(200000);
    SetSignal(SIGNAL_THREAD_TERMINATE);
    ALOGV("DEBUG(%s): DONE", __func__);
}

ExynosCameraHWInterface2::StreamThread::~StreamThread()
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::StreamThread::setParameter(stream_parameters_t * new_parameters)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);

    m_tempParameters = new_parameters;

    SetSignal(SIGNAL_STREAM_CHANGE_PARAMETER);

    // TODO : return synchronously (after setting parameters asynchronously)
    usleep(50000);
}

void ExynosCameraHWInterface2::StreamThread::applyChange()
{
    memcpy(&m_parameters, m_tempParameters, sizeof(stream_parameters_t));

    ALOGD("DEBUG(%s):  Applying Stream paremeters  width(%d), height(%d)",
            __FUNCTION__, m_parameters.outputWidth, m_parameters.outputHeight);
}

void ExynosCameraHWInterface2::StreamThread::release()
{
    ALOGV("DEBUG(%s):", __func__);

    SetSignal(SIGNAL_THREAD_RELEASE);

    // TODO : return synchronously (after releasing asynchronously)
    usleep(200000);
    while (m_index != 255)  // temp.. To make smarter..
        usleep(200000);
    ALOGV("DEBUG(%s): DONE", __func__);
}

int ExynosCameraHWInterface2::StreamThread::findBufferIndex(void * bufAddr)
{
    int index;
    for (index = 0 ; index < m_parameters.numSvcBuffers ; index++) {
        if (m_parameters.svcBuffers[index].virt.extP[0] == bufAddr)
            return index;
    }
    return -1;
}

int ExynosCameraHWInterface2::createIonClient(ion_client ionClient)
{
    if (ionClient == 0) {
        ionClient = ion_client_create();
        if (ionClient < 0) {
            ALOGE("[%s]src ion client create failed, value = %d\n", __FUNCTION__, ionClient);
            return 0;
        }
    }

    return ionClient;
}

int ExynosCameraHWInterface2::deleteIonClient(ion_client ionClient)
{
    if (ionClient != 0) {
        if (ionClient > 0) {
            ion_client_destroy(ionClient);
        }
        ionClient = 0;
    }

    return ionClient;
}

int ExynosCameraHWInterface2::allocCameraMemory(ion_client ionClient, ExynosBuffer *buf, int iMemoryNum)
{
    int ret = 0;
    int i = 0;

    if (ionClient == 0) {
        ALOGE("[%s] ionClient is zero (%d)\n", __FUNCTION__, ionClient);
        return -1;
    }

    for (i=0;i<iMemoryNum;i++) {
        if (buf->size.extS[i] == 0) {
            break;
        }

        buf->fd.extFd[i] = ion_alloc(ionClient, \
                                      buf->size.extS[i], 0, ION_HEAP_EXYNOS_MASK,0);
        if ((buf->fd.extFd[i] == -1) ||(buf->fd.extFd[i] == 0)) {
            ALOGE("[%s]ion_alloc(%d) failed\n", __FUNCTION__, buf->size.extS[i]);
            buf->fd.extFd[i] = -1;
            freeCameraMemory(buf, iMemoryNum);
            return -1;
        }

        buf->virt.extP[i] = (char *)ion_map(buf->fd.extFd[i], \
                                        buf->size.extS[i], 0);
        if ((buf->virt.extP[i] == (char *)MAP_FAILED) || (buf->virt.extP[i] == NULL)) {
            ALOGE("[%s]src ion map failed(%d)\n", __FUNCTION__, buf->size.extS[i]);
            buf->virt.extP[i] = (char *)MAP_FAILED;
            freeCameraMemory(buf, iMemoryNum);
            return -1;
        }
        ALOGV("allocCameraMem : [%d][0x%08x] size(%d)", i, (unsigned int)(buf->virt.extP[i]), buf->size.extS[i]);
    }

    return ret;
}

void ExynosCameraHWInterface2::freeCameraMemory(ExynosBuffer *buf, int iMemoryNum)
{

    int i =0 ;

    for (i=0;i<iMemoryNum;i++) {
        if (buf->fd.extFd[i] != -1) {
            if (buf->virt.extP[i] != (char *)MAP_FAILED) {
                ion_unmap(buf->virt.extP[i], buf->size.extS[i]);
            }
            ion_free(buf->fd.extFd[i]);
        }
        buf->fd.extFd[i] = -1;
        buf->virt.extP[i] = (char *)MAP_FAILED;
        buf->size.extS[i] = 0;
    }
}

void ExynosCameraHWInterface2::initCameraMemory(ExynosBuffer *buf, int iMemoryNum)
{
    int i =0 ;
    for (i=0;i<iMemoryNum;i++) {
        buf->virt.extP[i] = (char *)MAP_FAILED;
        buf->fd.extFd[i] = -1;
        buf->size.extS[i] = 0;
    }
}




static camera2_device_t *g_cam2_device;

static int HAL2_camera_device_close(struct hw_device_t* device)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    if (device) {
        camera2_device_t *cam_device = (camera2_device_t *)device;
        delete static_cast<ExynosCameraHWInterface2 *>(cam_device->priv);
        free(cam_device);
        g_cam2_device = 0;
    }
    return 0;
}

static inline ExynosCameraHWInterface2 *obj(const struct camera2_device *dev)
{
    return reinterpret_cast<ExynosCameraHWInterface2 *>(dev->priv);
}

static int HAL2_device_set_request_queue_src_ops(const struct camera2_device *dev,
            const camera2_request_queue_src_ops_t *request_src_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setRequestQueueSrcOps(request_src_ops);
}

static int HAL2_device_notify_request_queue_not_empty(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->notifyRequestQueueNotEmpty();
}

static int HAL2_device_set_frame_queue_dst_ops(const struct camera2_device *dev,
            const camera2_frame_queue_dst_ops_t *frame_dst_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setFrameQueueDstOps(frame_dst_ops);
}

static int HAL2_device_get_in_progress_count(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->getInProgressCount();
}

static int HAL2_device_flush_captures_in_progress(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->flushCapturesInProgress();
}

static int HAL2_device_construct_default_request(const struct camera2_device *dev,
            int request_template, camera_metadata_t **request)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->constructDefaultRequest(request_template, request);
}

static int HAL2_device_allocate_stream(
            const struct camera2_device *dev,
            // inputs
            uint32_t width,
            uint32_t height,
            int      format,
            const camera2_stream_ops_t *stream_ops,
            // outputs
            uint32_t *stream_id,
            uint32_t *format_actual,
            uint32_t *usage,
            uint32_t *max_buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->allocateStream(width, height, format, stream_ops,
                                    stream_id, format_actual, usage, max_buffers);
}


static int HAL2_device_register_stream_buffers(const struct camera2_device *dev,
            uint32_t stream_id,
            int num_buffers,
            buffer_handle_t *buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->registerStreamBuffers(stream_id, num_buffers, buffers);
}

static int HAL2_device_release_stream(
        const struct camera2_device *dev,
            uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->releaseStream(stream_id);
}

static int HAL2_device_allocate_reprocess_stream(
           const struct camera2_device *dev,
            uint32_t width,
            uint32_t height,
            uint32_t format,
            const camera2_stream_in_ops_t *reprocess_stream_ops,
            // outputs
            uint32_t *stream_id,
            uint32_t *consumer_usage,
            uint32_t *max_buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->allocateReprocessStream(width, height, format, reprocess_stream_ops,
                                    stream_id, consumer_usage, max_buffers);
}

static int HAL2_device_release_reprocess_stream(
        const struct camera2_device *dev,
            uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->releaseReprocessStream(stream_id);
}

static int HAL2_device_trigger_action(const struct camera2_device *dev,
           uint32_t trigger_id,
            int ext1,
            int ext2)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->triggerAction(trigger_id, ext1, ext2);
}

static int HAL2_device_set_notify_callback(const struct camera2_device *dev,
            camera2_notify_callback notify_cb,
            void *user)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setNotifyCallback(notify_cb, user);
}

static int HAL2_device_get_metadata_vendor_tag_ops(const struct camera2_device*dev,
            vendor_tag_query_ops_t **ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->getMetadataVendorTagOps(ops);
}

static int HAL2_device_dump(const struct camera2_device *dev, int fd)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->dump(fd);
}





static int HAL2_getNumberOfCameras()
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 1;
}


static int HAL2_getCameraInfo(int cameraId, struct camera_info *info)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    static camera_metadata_t * mCameraInfo = NULL;
    status_t res;

    info->facing = CAMERA_FACING_BACK;
    info->orientation = 0;
    info->device_version = HARDWARE_DEVICE_API_VERSION(2, 0);
    if (mCameraInfo==NULL) {
        res = constructStaticInfo(&mCameraInfo, true);
        if (res != OK) {
            ALOGE("%s: Unable to allocate static info: %s (%d)",
                    __FUNCTION__, strerror(-res), res);
            return res;
        }
        res = constructStaticInfo(&mCameraInfo, false);
        if (res != OK) {
            ALOGE("%s: Unable to fill in static info: %s (%d)",
                    __FUNCTION__, strerror(-res), res);
            return res;
        }
    }
    info->static_camera_characteristics = mCameraInfo;
    return NO_ERROR;
}

#define SET_METHOD(m) m : HAL2_device_##m

static camera2_device_ops_t camera2_device_ops = {
        SET_METHOD(set_request_queue_src_ops),
        SET_METHOD(notify_request_queue_not_empty),
        SET_METHOD(set_frame_queue_dst_ops),
        SET_METHOD(get_in_progress_count),
        SET_METHOD(flush_captures_in_progress),
        SET_METHOD(construct_default_request),
        SET_METHOD(allocate_stream),
        SET_METHOD(register_stream_buffers),
        SET_METHOD(release_stream),
        SET_METHOD(allocate_reprocess_stream),
        SET_METHOD(release_reprocess_stream),
        SET_METHOD(trigger_action),
        SET_METHOD(set_notify_callback),
        SET_METHOD(get_metadata_vendor_tag_ops),
        SET_METHOD(dump),
};

#undef SET_METHOD


static int HAL2_camera_device_open(const struct hw_module_t* module,
                                  const char *id,
                                  struct hw_device_t** device)
{
    ALOGE(">>> I'm Samsung's CameraHAL_2 <<<");

    int cameraId = atoi(id);
    if (cameraId < 0 || cameraId >= HAL2_getNumberOfCameras()) {
        ALOGE("ERR(%s):Invalid camera ID %s", __FUNCTION__, id);
        return -EINVAL;
    }

    if (g_cam2_device) {
        if (obj(g_cam2_device)->getCameraId() == cameraId) {
            ALOGV("DEBUG(%s):returning existing camera ID %s", __FUNCTION__, id);
            goto done;
        } else {
            ALOGE("ERR(%s):Cannot open camera %d. camera %d is already running!",
                    __FUNCTION__, cameraId, obj(g_cam2_device)->getCameraId());
            return -ENOSYS;
        }
    }

    g_cam2_device = (camera2_device_t *)malloc(sizeof(camera2_device_t));
    if (!g_cam2_device)
        return -ENOMEM;

    g_cam2_device->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam2_device->common.version = CAMERA_DEVICE_API_VERSION_2_0;
    g_cam2_device->common.module  = const_cast<hw_module_t *>(module);
    g_cam2_device->common.close   = HAL2_camera_device_close;

    g_cam2_device->ops = &camera2_device_ops;

    ALOGV("DEBUG(%s):open camera2 %s", __FUNCTION__, id);

    g_cam2_device->priv = new ExynosCameraHWInterface2(cameraId, g_cam2_device);

done:
    *device = (hw_device_t *)g_cam2_device;
    ALOGV("DEBUG(%s):opened camera2 %s (%p)", __FUNCTION__, id, *device);

    return 0;
}


static hw_module_methods_t camera_module_methods = {
            open : HAL2_camera_device_open
};

extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
      common : {
          tag                : HARDWARE_MODULE_TAG,
          module_api_version : CAMERA_MODULE_API_VERSION_2_0,
          hal_api_version    : HARDWARE_HAL_API_VERSION,
          id                 : CAMERA_HARDWARE_MODULE_ID,
          name               : "Exynos Camera HAL2",
          author             : "Samsung Corporation",
          methods            : &camera_module_methods,
          dso:                NULL,
          reserved:           {0},
      },
      get_number_of_cameras : HAL2_getNumberOfCameras,
      get_camera_info       : HAL2_getCameraInfo
    };
}

}; // namespace android
