/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _CORE_H_
#define _CORE_H_

#include <QObject>
#include <QProcess> // For QProcess::ProcessError
#include "mediadata.h"
#include "mediasettings.h"
#include "config.h"


class MplayerProcess;
class MplayerWindow;
class QSettings;

#ifdef Q_OS_WIN
class WinScreenSaver;
#endif

class Core : public QObject
{
    Q_OBJECT
    
public:
	enum State { Stopped = 0, Playing = 1, Paused = 2 };

    Core( MplayerWindow *mpw, QWidget* parent = 0 );
    ~Core();

    MediaData mdat;
	MediaSettings mset;

    QString mplayer_log;

	//! Return the current state
	State state() { return _state; };

	//! Return a string with the name of the current state,
	//! so it can be printed on debugging messages.
	QString stateToString();

protected:
	//! Change the current state (Stopped, Playing or Paused)
	//! And sends the stateChanged() signal.
	void setState(State s);
    
public slots:
	//! Generic open, with autodetection of type
	void open(QString file, int seek=-1); 
	void openFile(QString filename, int seek=-1);
	void openStream(QString name);
	/*
	void openDVD( bool from_folder, QString directory = "");
	void openDVD(); // Plays title 1
	void openDVD(int title = 1);
	*/
	void openDVD(QString dvd_url);
	void openVCD(int title = -1);
	void openAudioCD(int title = -1);

	void loadSub(const QString & sub);
	void unloadSub();

	//! Forces to use the specified subtitle file. It's not loaded immediately but stored
	//! and will be used for the next video. After that the variable is cleared.  
	void setInitialSubtitle(const QString & subtitle_file) { initial_subtitle = subtitle_file; };

	void loadAudioFile(const QString & audiofile);
	void unloadAudioFile();

    void stop(); 
    void play();
	void play_or_pause();
    void pause_and_frame_step();
	void pause();
	void frameStep();
	void screenshot();

	//! Public restart, for the GUI.
	void restart();

	//! Reopens the file (no restart)
	void reload();

    void goToPos( int perc );
    void goToSec( double sec );

	void toggleRepeat();
	void toggleRepeat(bool b);

	void toggleFlip();
	void toggleFlip(bool b);

	// Audio filters
	void toggleKaraoke();
	void toggleKaraoke(bool b);
	void toggleExtrastereo();
	void toggleExtrastereo(bool b);
	void toggleVolnorm();
	void toggleVolnorm(bool b);

	void setAudioChannels(int channels);
	void setStereoMode(int mode);

	// Video filters
	void toggleAutophase();
	void toggleAutophase(bool b);
	void toggleDeblock();
	void toggleDeblock(bool b);
	void toggleDering();
	void toggleDering(bool b);
	void toggleNoise();
	void toggleNoise(bool b);
	void togglePostprocessing();
	void togglePostprocessing(bool b);
	void changeDenoise(int);
#if NEW_ASPECT_CODE
	void changeLetterbox(bool);
#endif
	void changeUpscale(bool);

	void seek(int secs);
	void sforward(); 	// + 10 seconds
	void srewind(); 	// - 10 seconds
    void forward(); 	// + 1 minute
    void rewind(); 		// -1 minute
    void fastforward();	// + 10 minutes
    void fastrewind();	// - 10 minutes
	void forward(int secs);
	void rewind(int secs);
	void wheelUp();
	void wheelDown();

	void setSpeed( double value );
	void incSpeed();
	void decSpeed();
	void doubleSpeed();
	void halveSpeed();
	void normalSpeed();

    void setVolume(int volume, bool force = false);
	void switchMute();
	void mute(bool b);
	void incVolume();
	void decVolume();

	void setBrightness(int value);
	void setContrast(int value);
	void setGamma(int value);
	void setHue(int value);
	void setSaturation(int value);

	void incBrightness();
	void decBrightness();
	void incContrast();
	void decContrast();
	void incGamma();
	void decGamma();
	void incHue();
	void decHue();
	void incSaturation();
	void decSaturation();

	void incSubDelay();
	void decSubDelay();

	void incAudioDelay();
	void decAudioDelay();

	void incSubPos();
	void decSubPos();

	void changeSubScale(double value);
	void incSubScale();
	void decSubScale();

	//! Select next line in subtitle file
	void incSubStep();
	//! Select previous line in subtitle file
	void decSubStep();

	//! Audio equalizer
	void setAudioEqualizer(AudioEqualizerList values, bool restart = false);
	void setAudioAudioEqualizerRestart(AudioEqualizerList values) { setAudioEqualizer(values, true); };
	void updateAudioEqualizer();

	void setAudioEq0(int value);
	void setAudioEq1(int value);
	void setAudioEq2(int value);
	void setAudioEq3(int value);
	void setAudioEq4(int value);
	void setAudioEq5(int value);
	void setAudioEq6(int value);
	void setAudioEq7(int value);
	void setAudioEq8(int value);
	void setAudioEq9(int value);

	void changeDeinterlace(int);
    void changeSubtitle(int);
	void nextSubtitle();
	void changeAudio(int ID, bool allow_restart = true);
	void nextAudio();
	void changeTitle(int);
	void changeChapter(int);
	void prevChapter();
	void nextChapter();
	void changeAngle(int);
	void changeAspectRatio(int);
	void changeOSD(int);
	void nextOSD();

	void changeSize(int); // Size of the window
	void toggleDoubleSize();
	void changePanscan(double); // Zoom on mplayerwindow

	void changeRotate(int r);

	void incPanscan();
	void decPanscan();
	void resetPanscan();

	void changeUseAss(bool);
	void toggleClosedCaption(bool);
	void toggleForcedSubsOnly(bool);

	void visualizeMotionVectors(bool);

    // Pass a command to mplayer by stdin:
    void tellmp(const QString & command);

public:
	//! Returns the number of the first chapter in mkv
	//! files. In some versions of mplayer is 0, in others 1
	static int mkv_first_chapter();
	static int dvd_first_chapter();

protected slots:
    void changeCurrentSec(double sec);
    void changePause();
	void gotWindowResolution( int w, int h );
	void gotNoVideo();
	void gotVO(QString);
	void gotAO(QString);
	void gotStartingTime(double);

	void finishRestart();
    void processFinished();
	void fileReachedEnd();
    
	void updateLog(QString line);

	void displayMessage(QString text);
	void displayScreenshotName(QString filename);

	void streamTitleAndUrlChanged(QString,QString);
	void autosaveMplayerLog();
	
	void watchState(Core::State state);

	//! Called when a video has just started to play.
	//! This function checks if the codec of video is ffh264 and if
	//! the resolution is HD
	void checkIfVideoIsHD();

#if DELAYED_AUDIO_SETUP_ON_STARTUP
	void initAudioTrack();
#endif

protected:
	void playNewFile(QString file, int seek=-1);
	void restartPlay();
	void initPlaying(int seek=-1);
	void newMediaPlaying();

    void startMplayer(QString file, double seek = -1 );
	void stopMplayer();

#ifndef NO_USE_INI_FILES
	bool checkHaveSettingsSaved(QString filename);
	void saveMediaInfo();
	void loadMediaInfo(QString filename);
#endif

    void initializeMenus();
	void updateWidgets();

#if SCALE_ASS_SUBS
	//! Returns true if changing the subscale requires to restart mplayer
	bool subscale_need_restart();
#endif

signals:
	void aboutToStartPlaying(); // Signal emited just before to start mplayer
	void mediaLoaded();
	void mediaInfoChanged();
    void stateChanged(Core::State state);
	void mediaStartPlay();
	void mediaFinished(); // Media has arrived to the end.
	void mediaStoppedByUser();
	void showMessage(QString text);
	void menusNeedInitialize();
	void widgetsNeedUpdate();
	void videoEqualizerNeedsUpdate();
	void audioEqualizerNeedsUpdate();
	void showTime(double sec);
	void posChanged(int); // To connect a slider
	void showFrame(int frame);
	void needResize(int w, int h);
	void noVideo();
	void volumeChanged(int);

	//! MPlayer started but finished with exit code != 0
	void mplayerFinishedWithError(int exitCode);

	//! MPlayer didn't started or crashed
	void mplayerFailed(QProcess::ProcessError error);

	// Resend signal from mplayerprocess:
	void failedToParseMplayerVersion(QString line_with_mplayer_version);

protected:
    MplayerProcess * proc;
    MplayerWindow * mplayerwindow;

#ifndef NO_USE_INI_FILES
	QSettings * file_settings;
#endif

#ifdef Q_OS_WIN
	WinScreenSaver * win_screensaver;
#endif
    
private:
	// Some variables to proper restart
	bool we_are_restarting;

	bool just_loaded_external_subs;
	bool just_unloaded_external_subs;
	State _state;
	bool change_volume_after_unpause;

	QString initial_subtitle;
};
    
#endif
