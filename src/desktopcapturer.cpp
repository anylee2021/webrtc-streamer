/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** screencapturer.cpp
**
** -------------------------------------------------------------------------*/

#ifdef USE_X11

#include "rtc_base/logging.h"

#include "desktopcapturer.h"

void DesktopCapturer::OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame) {
	
	RTC_LOG(LS_INFO) << "DesktopCapturer:OnCaptureResult";
	
	if (result == webrtc::DesktopCapturer::Result::SUCCESS) {
		int width = frame->stride() / webrtc::DesktopFrame::kBytesPerPixel;
		int height = frame->rect().height();

		webrtc::scoped_refptr<webrtc::I420Buffer> I420buffer = webrtc::I420Buffer::Create(width, height);

		const int conversionResult = libyuv::ConvertToI420(frame->data(), 0,
			I420buffer->MutableDataY(), I420buffer->StrideY(),
			I420buffer->MutableDataU(), I420buffer->StrideU(),
			I420buffer->MutableDataV(), I420buffer->StrideV(),
			0, 0,
			width, height,
			I420buffer->width(), I420buffer->height(),
			libyuv::kRotate0, ::libyuv::FOURCC_ARGB);									
				
		if (conversionResult >= 0) {
			webrtc::VideoFrame videoFrame(I420buffer, webrtc::VideoRotation::kVideoRotation_0, webrtc::TimeMicros());
			if ( (m_height == 0) && (m_width == 0) ) {
				m_broadcaster.OnFrame(videoFrame);	

			} else {
				int height = m_height;
				int width = m_width;
				if (height == 0) {
					height = (videoFrame.height() * width) / videoFrame.width();
				}
				else if (width == 0) {
					width = (videoFrame.width() * height) / videoFrame.height();
				}
				int stride_y = width;
				int stride_uv = (width + 1) / 2;
				webrtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer = webrtc::I420Buffer::Create(width, height, stride_y, stride_uv, stride_uv);
				scaled_buffer->ScaleFrom(*videoFrame.video_frame_buffer()->ToI420());

	            webrtc::VideoFrame frame = webrtc::VideoFrame::Builder()
					.set_video_frame_buffer(scaled_buffer)
					.set_rotation(webrtc::kVideoRotation_0)
					.set_timestamp_us(webrtc::TimeMicros())
					.build();
				
						
				m_broadcaster.OnFrame(frame);
			}
		} else {
			RTC_LOG(LS_ERROR) << "DesktopCapturer:OnCaptureResult conversion error:" << conversionResult;
		}				

	} else {
		RTC_LOG(LS_ERROR) << "DesktopCapturer:OnCaptureResult capture error:" << (int)result;
	}
}
		
void DesktopCapturer::CaptureThread() {
	RTC_LOG(LS_INFO) << "DesktopCapturer:Run start";
	while (IsRunning()) {
		m_capturer->CaptureFrame();
	}
	RTC_LOG(LS_INFO) << "DesktopCapturer:Run exit";
}
bool DesktopCapturer::Start() {
	m_isrunning = true;
	m_capturethread = std::thread(&DesktopCapturer::CaptureThread, this); 
	m_capturer->Start(this);
	return true;
}
		
void DesktopCapturer::Stop() {
	m_isrunning = false;
	m_capturethread.join(); 
}
		
#endif

