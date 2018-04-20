/*  This file is part of YUView - The YUV player with advanced analytics toolset
*   <https://github.com/IENT/YUView>
*   Copyright (C) 2015  Institut f�r Nachrichtentechnik, RWTH Aachen University, GERMANY
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 3 of the License, or
*   (at your option) any later version.
*
*   In addition, as a special exception, the copyright holders give
*   permission to link the code of portions of this program with the
*   OpenSSL library under certain conditions as described in each
*   individual source file, and distribute linked combinations including
*   the two.
*   
*   You must obey the GNU General Public License in all respects for all
*   of the code used other than OpenSSL. If you modify file(s) with this
*   exception, you may extend this exception to your version of the
*   file(s), but you are not obligated to do so. If you do not wish to do
*   so, delete this exception statement from your version. If you delete
*   this exception statement from all source files in the program, then
*   also delete it here.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FILESOURCEFFMPEGFILE_H
#define FILESOURCEFFMPEGFILE_H

#include "fileSource.h"
#include "FFMpegLibrariesHandling.h"
#include "videoHandlerYUV.h"

/* This class can use the ffmpeg libraries (libavcodec) to read from any packetized file.
*/
class fileSourceFFmpegFile : public QObject
{
  Q_OBJECT

public:
  fileSourceFFmpegFile();
  fileSourceFFmpegFile(const QString &filePath) : fileSourceFFmpegFile() { openFile(filePath); }
  ~fileSourceFFmpegFile();

  // Load the ffmpeg libraries and try to open the file. The fileSource will install a watcher for the file.
  // Return false if anything goes wrong.
  bool openFile(const QString &filePath, fileSourceFFmpegFile *other=nullptr);
  
  // Is the file at the end?
  // TODO: How do we do this?
  bool atEnd() const { return endOfFile; }

  // Get properties of the bitstream
  double getFramerate() { return frameRate; }
  QSize getSequenceSizeSamples() { return frameSize; }
  YUV_Internals::yuvPixelFormat getPixelFormat() { return pixelFormat; }

  // Get the next NAL unit (everything excluding the start code) or the next packet.
  // Do not mix calls to these two functions when reading a file.
  QByteArray getNextNALUnit(uint64_t *pts=nullptr);
  // Return a reference to the packet. This class has the packet and handels it.
  AVPacketWrapper getNextPacket();
  // Return the raw extradata (in avformat format containing the parameter sets)
  QByteArray getExtradata();
  // Return a list containing the raw data of all parameter set NAL units
  QList<QByteArray> getParameterSets();

  // File watching
  void updateFileWatchSetting();
  bool isFileChanged() { bool b = fileChanged; fileChanged = false; return b; }

  bool seekToPTS(int64_t pts);
  int64_t getMaxPTS();

  int getNumberFrames() const { return nrFrames; }
  AVCodecID getCodec() { return video_stream.getCodecID(); }
  AVCodecParametersWrapper getVideoCodecPar() { return video_stream.get_codecpar(); }

  // Look through the keyframes and find the closest one before (or equal)
  // the given frameIdx where we can start decoding
  int getClosestSeekableDTSBefore(int frameIdx, int &seekToFrameIdx) const;
  
private slots:
  void fileSystemWatcherFileChanged(const QString &path) { Q_UNUSED(path); fileChanged = true; }

protected:

  FFmpegVersionHandler ff;          //< Access to the libraries independent of their version
  AVFormatContextWrapper fmt_ctx;
  void openFileAndFindVideoStream(QString fileName);
  bool goToNextVideoPacket();
  AVPacketWrapper pkt;              //< A place for the curren (frame) input buffer
  bool endOfFile;                   //< Are we at the end of file (draining mode)?
  // Seek the stream to the given pts value, flush the decoder and load the first packet so
  // that we are ready to start decoding from this pts.
  int64_t duration;
  AVRational timeBase;
  AVStreamWrapper video_stream;
  double frameRate;
  QSize frameSize;
  YUV_Internals::yuvPixelFormat pixelFormat;
  YUV_Internals::ColorConversion colorConversionType;

  // Watch the opened file for modifications
  QFileSystemWatcher fileWatcher;
  bool fileChanged;

  QString   fullFilePath;
  QFileInfo fileInfo;
  bool      isFileOpened;

  // In order to translate from frames to PTS, we need to count the frames and keep a list of
  // the PTS values of keyframes that we can start decoding at.
  void scanBitstream();
  int nrFrames;

  // Private struct for navigation. We index frames by frame number and FFMpeg uses the pts.
  // This connects both values.
  struct pictureIdx
  {
    pictureIdx(int64_t frame, int64_t pts) : frame(frame), pts(pts) {}
    int64_t frame;
    int64_t pts;
  };

  // These are filled after opening a file (after scanBitstream was called)
  QList<pictureIdx> keyFrameList;  //< A list of pairs (frameNr, PTS) that we can seek to.
  //pictureIdx getClosestSeekableFrameNumberBeforeBefore(int frameIdx);

  // For parsing NAL units from the compressed data:
  QByteArray currentPacketData;
  int posInFile;
  bool loadNextPacket;
  int posInData;

};

#endif // FILESOURCEFFMPEGFILE_H