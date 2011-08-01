/*
 *  SampleRecorder.cpp
 *  ofxQTAudioSaver_Monobanda
 *
 *  Created by Arne Boon on 11/13/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "SampleRecorder.h"
#include "stdio.h"

//-------------------------------------------------------------
SampleRecorder::~SampleRecorder(){
	if (bIsRecording) stopRecording();
	
	//--NOTICE: commented out to prevent crash
	//ofSoundStreamStop();
	//ofSoundStreamClose();
	
	// clear memory
	delete [] left;
	delete [] right;
	
	delete [] mAudio_audioBuffer;
	delete [] mAudio_audioBufferFloat;
		
	cout << "SAMPLE RECORDER: clear memory" << endl;
}

//--------------------------------------------------------------
//void SampleRecorder::setup(ofxSoundStream & masterMAudio, SampleStreamPlayback & sampleStreamPlayback){	
void SampleRecorder::setup(ofxSoundStream & masterMAudio){	
	
	left = new float[BUFFER_SIZE];
	right = new float[BUFFER_SIZE];
	
	for (int i = 0; i < BUFFER_SIZE; i++) {
		left[i] = 0;
		right[i] = 0;
	}
	
	// create an empty buffer for device 1 (2 channels)
	int mAudio_audioBufferInt = SAMPLE_RATE * RECORDING_DURATION * RECORDING_CHANNELS;
	mAudio_audioBuffer = new UInt8[mAudio_audioBufferInt];
	mAudio_audioBufferFloat = new float[mAudio_audioBufferInt];
	
	bIsRecording = false;
	
	ofAddListener(masterMAudio.audioReceivedEvent, this, &SampleRecorder::audioInputListener);
	//ofAddListener(sampleStreamPlayback.onNextStepEvent, this, &SampleRecorder::startRecording);
	
	recordingFont.loadFont("MONACO.TTF", 16, true, true, false);
}

//--------------------------------------------------------------
void SampleRecorder::update(){
	//
}

//--------------------------------------------------------------
void SampleRecorder::draw(){
	
	ofSetColor(255,255,255);
	recordingFont.drawString("U heeft 60 seconden om een boodschap in te spreken" , 100, 240);
	recordingFont.drawString("Toets # om de boodschap voortijdig te beeindigen" , 100, 280);
	
	/*
	// draw the left:
	ofSetColor(100,100,100);
	ofRect(100,100,256,200);
	ofSetColor(255,255,255);
	for (int i = 0; i < 256; i++){
		ofLine(100+i,200,100+i,200+left[i]*100.0f);
	}
	
	// draw the right:
	ofSetColor(100,100,100);
	ofRect(400,100,256,200);
	ofSetColor(255,255,255);
	for (int i = 0; i < 256; i++){
		ofLine(400+i,200,400+i,200+right[i]*100.0f);
	}
	*/
	
	// draw the recording
	int stepsSamples = 60;
	ofSetColor(100,100,100);
	ofRect(100, 340, 44100/stepsSamples,200);
	for (int i = 0; i < RECORDING_DURATION * SAMPLE_RATE * RECORDING_CHANNELS; i+=stepsSamples){
		
		float xPos = 100+i/(RECORDING_DURATION * RECORDING_CHANNELS * stepsSamples);
		float offset = mAudio_audioBufferFloat[i]*100;
		
		ofSetColor(255,100,100);
		ofLine(xPos,440,xPos,440+offset);
	}
	
	ofSetColor(255, 255, 255);
}

//-START - STOP RECORDING-------------------------------------------------------------
//--------------------------------------------------------------
//void SampleRecorder::startRecording(int & _stepPosition){
void SampleRecorder::startRecording(){
	
	//cout << "SAMPLE_RECORDER >> receives stepPosition: " << _stepPosition << endl;
	
	SF_INFO mAudio_info;
	
	//if(!bIsRecording && _stepPosition == 2){
	if(!bIsRecording){
		
		//--clear buffers
		for (int i = 0; i < BUFFER_SIZE; i++) {
			left[i] = 0;
			right[i] = 0;
		}
		
		for (int i = 0; i < RECORDING_DURATION * SAMPLE_RATE * RECORDING_CHANNELS; i++) {
			mAudio_audioBufferFloat[i] = 0;
			mAudio_audioBufferFloat[i] = 0;
		}
		
		mAudio_info.frames = SAMPLE_RATE * 60;
		mAudio_info.samplerate = SAMPLE_RATE;
		mAudio_info.channels = RECORDING_CHANNELS;
		mAudio_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
		
		lastRecordingFileName = "recordings/" 
								+ ofToString( ofGetYear() ) + "_"
								+ ofToString( ofGetMonth() ) + "_"
								+ ofToString( ofGetDay() ) + "_"
								+ ofToString( ofGetHours() ) + "_"
								+ ofToString( ofGetMinutes() ) + "_"
								+ ofToString( ofGetSeconds() ) +
								+ ".wav";
		
		ofNotifyEvent(onLastRecordingFileName, lastRecordingFileName, this);
		
		mAudio_recorder = sf_open(ofToDataPath(lastRecordingFileName).c_str(), SFM_WRITE, &mAudio_info);
		
		mAudio_bufferCounter = 0;
		bIsRecording = true;

		cout << "SAMPLE RECORDER >> START RECORDING: " << lastRecordingFileName << endl;
	}
}

//--------------------------------------------------------------
void SampleRecorder::stopRecording(){
	
	
	if(bIsRecording){
		bIsRecording = false;
		
		sf_close(mAudio_recorder);
				
		cout << "SAMPLE RECORDER >> STOP RECORDING: " << lastRecordingFileName << endl;
	}
	
	argsRecording.bIsRecording = false;
	ofNotifyEvent(onFinishedRecordingEvent, argsRecording, this);

}

//--------------------------------------------------------------
void SampleRecorder::audioInputListener(ofxAudioEventArgs &args){	
			
	if(bIsRecording){
		
		int maxBuffers = SAMPLE_RATE * RECORDING_DURATION / args.bufferSize;
			
		//if(args.deviceName == "M-Audio, Inc.: M-Audio Fast Track Pro USB"){
			
			//from stereo to mono over 2 channels
			for (int i = 0; i < args.bufferSize; i++) {
				args.buffer[i*2+1] = args.buffer[i*2];
			}
						
			if(mAudio_bufferCounter < maxBuffers){
				for (int i = 0; i < args.bufferSize; i++){
					left[i] = args.buffer[i*2];
					right[i] = args.buffer[i*2+1];
										
					// interleave samples (change 2 integer) [[no need to fill since only mAudio_audioBufferFloat is used lateron]]
					//mAudio_audioBuffer[mAudio_bufferCounter * args.bufferSize * args.nChannels+i*args.nChannels] =  args.buffer[i*args.nChannels] * 256;
					//mAudio_audioBuffer[mAudio_bufferCounter * args.bufferSize * args.nChannels+i*args.nChannels+1] =  args.buffer[i*args.nChannels+1] * 256;
					
					mAudio_audioBufferFloat[mAudio_bufferCounter * args.bufferSize * args.nChannels+i*args.nChannels] =  args.buffer[i*args.nChannels];
					mAudio_audioBufferFloat[mAudio_bufferCounter * args.bufferSize * args.nChannels+i*args.nChannels+1] =  args.buffer[i*args.nChannels+1];
				}
			
				sf_write_float(mAudio_recorder, args.buffer, args.bufferSize*2);
				
				mAudio_bufferCounter++;
			}
		//}
		
		if((mAudio_bufferCounter == maxBuffers)){
			this->stopRecording();
		}
		
	}
} 