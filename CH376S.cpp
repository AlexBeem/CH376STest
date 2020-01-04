#include "CH376S.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
CH376S_UART::CH376S_UART(Stream& s)
{
  workStream = &s;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::begin()
{

  if(!checkConnection()) // не работает железка
    return false;

  return prepare();
  //return true;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CH376S_UART::sendStartFrame()
{
  workStream->write(CH376_CMD_FRAME_START_1);
  workStream->write(CH376_CMD_FRAME_START_2);  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CH376S_UART::setFileName(const String& fileName)
{
  sendStartFrame();
  workStream->write(CH376_CMD_SET_FILENAME);
  workStream->write(CH376_CMD_SET_FILENAME);         // Every filename must have this byte to indicate the start of the file name.
  workStream->print(fileName);     // "fileName" is a variable that holds the name of the file.  eg. TEST.TXT
  workStream->write((uint8_t)0x00);   // The null byte indicates the end of the file name.
  delay(20);  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::mount()
{
  
  sendStartFrame();
  workStream->write(CH376_CMD_DISK_MOUNT);

  if(waitForResponse() && (getResponse() == CH376S_RETURN_OK))
       return true;

  return false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::waitForResponse(uint16_t timeout)
{
  bool bytesAvailable = true;
  uint16_t counter=0;
  while(!workStream->available())
  {
    delay(1);
    counter++;
    if(counter>timeout)
    {
      bytesAvailable = false;
      break;
    }
  }
  delay(1);
  return (bytesAvailable);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t CH376S_UART::getResponse()
{
  if (workStream->available())
    return workStream->read();
    
  return 0;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::isFlashDriveInserted()
{
  sendStartFrame();
  workStream->write(CH376_CMD_IS_DISK_CONNECTED);

  if(waitForResponse() && (getResponse() == CH376S_RETURN_OK))
       return true;

  return false;      
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CH376S_UART::resetALL()
{    
    sendStartFrame();
    workStream->write(CH376_CMD_RESET_ALL);
    
    delay(200); // (Wait for 35mS) - из даташита, НЕ РАБОТАЕТ такая маленькая задержка !!!!
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::checkConnection()
{
  uint8_t value = 0x01;
  
  sendStartFrame();
  workStream->write(CH376_CMD_CHECK_CONNECTION);
  workStream->write(value);
  
  if(waitForResponse() && (getResponse() == (255-value)))
      return true;

  return false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::setUSBMode(uint8_t value)
{
  bool result = false;
      
  sendStartFrame();
  workStream->write(CH376_CMD_USB_MODE);
  workStream->write(value);
  
  delay(20);
  
  if(workStream->available())
  {
    byte readed = workStream->read();

    if(readed == 0x51)
    {                                   
        readed = workStream->read();
        
        if(readed == 0x15)
          result = true;
        
    } 
  }

  return result;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::prepare()
{
   resetALL();
  
  if(!setUSBMode(0x06))
    return false;

  if(!isFlashDriveInserted())
    return false; 

  return mount();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::doOpenFile(const String& fileName)
{

  
  setFileName(fileName);
    
  sendStartFrame();
  workStream->write(CH376_CMD_FILE_OPEN);
  
  if(waitForResponse() && (getResponse() == CH376S_RETURN_OK))
       return true;

  return false;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::doCloseFile(bool updateFileSize)
{
  byte closeCmd = updateFileSize ? 0x01 : 0x00;
    
  sendStartFrame();
  workStream->write(CH376_CMD_FILE_CLOSE);
  workStream->write((byte)closeCmd); // closeCmd = 0x00 = close without updating file Size, 0x01 = close and update file Size

  if(waitForResponse() && (getResponse() == CH376S_RETURN_OK))
       return true;

  return false;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::doRemoveFile(const String& fileName)
{
  setFileName(fileName);
  
  sendStartFrame();
  workStream->write(CH376_CMD_FILE_REMOVE);
  
  if(waitForResponse() && (getResponse() == CH376S_RETURN_OK))
       return true;

  return false;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::doCreateFile(const String& fileName)
{
      
  setFileName(fileName);

  sendStartFrame();
  workStream->write(CH376_CMD_FILE_CREATE);
  
  if(waitForResponse() && (getResponse() == CH376S_RETURN_OK))
       return true;

  return false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t CH376S_UART::getFileSize()
{
  uint32_t fileSize = 0;

  sendStartFrame();
  workStream->write(CH376_CMD_FILE_SIZE);
  workStream->write(0x68);

  
  if(!waitForResponse())
    return fileSize;

  while(workStream->available() < 4)
    yield();
  
  fileSize = workStream->read();
  fileSize += (uint32_t(workStream->read())*255);  
  fileSize += (uint32_t(workStream->read())*255*255);
  fileSize += (uint32_t(workStream->read())*255*255*255);

  return fileSize;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::startByteRead(uint8_t numBytes)
{
  sendStartFrame();
  workStream->write(CH376_CMD_BYTE_READ);
  workStream->write((uint8_t)numBytes);
  workStream->write((uint8_t)0x00);
  workStream->flush();
  
  if(waitForResponse())
  {
    uint8_t resp = getResponse(); //read the CH376S message. If equal to 0x1D, data is present, so return true. Will return 0x14 if no data is present.
    if(resp == 0x1D)
      return true;
  }

  return false;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t CH376S_UART::readBytes(uint8_t* buffer)
{
    uint8_t readed = 0;
    
    sendStartFrame();
    workStream->write(CH376_CMD_RD_USB_DATA0);
    
    if(waitForResponse())
    {      
        uint8_t remaining = workStream->read();
        
        while(workStream->available() < (remaining-1))
          yield();
          
        while(workStream->available())
        {
            buffer[readed] = workStream->read();
            readed++;
        }
    }

  return readed;  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::continueRead()
{
  sendStartFrame();
  workStream->write(CH376_CMD_BYTE_RD_GO);
  
  if(waitForResponse() && (getResponse() == CH376S_RETURN_OK))
       return true;

  return false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::write(uint8_t* data, uint8_t dataLength)
{

  // This set of commands tells the CH376S module how many bytes to expect from the Arduino.  (defined by the "dataLength" variable)
  sendStartFrame();
  workStream->write(CH376_CMD_BYTE_WRITE);
  workStream->write((uint8_t) dataLength);
  workStream->write((uint8_t) 0x00);
  
  if(waitForResponse())
  {      
    if(getResponse() == 0x1E)
    {                
      // 0x1E indicates that the USB device is in write mode.
      sendStartFrame();
      workStream->write(CH376_CMD_WR_REQ_DATA);
      workStream->write(data,dataLength);
  
      if(waitForResponse())
      {   
        workStream->read();
      }
      
      sendStartFrame();
      workStream->write(CH376_CMD_BYTE_WR_GO);  // This is used to update the file size. Not sure if this is necessary for successful writing.
      
      if(waitForResponse())
      {   
        // wait for an acknowledgement from the CH376S module
        workStream->read();
      }

      return true;
    }
  }    

  return false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::exists(const String& fileName)
{
  if(doOpenFile(fileName))
  {
    doCloseFile();
    return true;
  }
  return false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::remove(const String& fileName)
{
  return doRemoveFile(fileName);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool CH376S_UART::doSeekSet(uint32_t pos)
{
  
  
  sendStartFrame();
  workStream->write(CH376_CMD_BYTE_LOCATE);

  uint8_t* ptr = (uint8_t*)&pos;
    
  for(uint8_t i=0;i<4;i++)
    workStream->write(*ptr++);   
  
  if(waitForResponse() && (getResponse() == CH376S_RETURN_OK))
    return true;
    
  
  return false;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
USBFile CH376S_UART::open(const String& fileName, bool createIfNotExists)
{
  USBFile result(this);
  result.open(fileName,createIfNotExists);
  return result;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// USBFile
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
USBFile::USBFile(CH376S_UART* d) : drive(d)
{
  reset();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
USBFile& USBFile::operator=(const USBFile& rhs)
{
  if(this == &rhs)
    return *this;
      
  this->openedFlag = rhs.openedFlag;
  this->fileSize = rhs.fileSize;
  this->fileReadIterator = rhs.fileReadIterator;
  this->drive = rhs.drive;

  return *this;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
USBFile::USBFile(const USBFile& rhs)
{
  openedFlag = rhs.openedFlag;
  fileSize = rhs.fileSize;
  fileReadIterator = rhs.fileReadIterator;
  drive = rhs.drive;
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
USBFile::~USBFile()
{
  if(openedFlag)
    close();
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void USBFile::reset()
{
  openedFlag = false;
  fileSize = 0;
  fileReadIterator = 0; 
  bufferIterator = -1;
  memset(readBuffer,0,sizeof(readBuffer));
  remain = 0;
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool USBFile::seekSet(uint32_t pos, bool atEnd)
{
  if(!openedFlag)
    return false;

  uint32_t newPos = pos;
  if(atEnd)
    newPos = fileSize - pos;

  return drive->doSeekSet(newPos);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void USBFile::open(const String& fileName, bool createIfNotExists)
{

  if(openedFlag)
    close();
    
  openedFlag = drive->doOpenFile(fileName);
  
  if(!openedFlag && createIfNotExists)
  {
    if(drive->doCreateFile(fileName))
    {
      openedFlag = drive->doOpenFile(fileName);
    }
  }

  if(openedFlag)
  {
    fileSize = drive->getFileSize();
  }
  
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool USBFile::close(bool updateFileSize)
{
  if(!openedFlag)
    return false;
    
  openedFlag = !drive->doCloseFile(updateFileSize);
  
  if(!openedFlag)
    reset();
      
  return !openedFlag;   
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool USBFile::available()
{
  return openedFlag && (fileReadIterator < fileSize);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t USBFile::read()
{

  if(!openedFlag || !available())
  {
    return 0;
  }

   // если во внутреннем буфере есть, что отдавать - отдаём.
   // иначе - читаем в буфер.
   // если не прочитано ничего - выходим.

   if(remain == 0)
   {
     // в буфере пусто - читаем
     if(drive->startByteRead(CH376S_BUFFER_SIZE))
     {
        remain = drive->readBytes(readBuffer);
        if((fileReadIterator + CH376S_BUFFER_SIZE) < fileSize && remain < CH376S_BUFFER_SIZE)
        {
          
          uint32_t diff = CH376S_BUFFER_SIZE - remain;
          uint32_t seekPos = fileReadIterator + (CH376S_BUFFER_SIZE - diff);
                    
          seekSet(seekPos);
        }
                
        bufferIterator = 0;
     }
   } // if(remain == 0)
   
   if(remain > 0)
   {
      remain--;
      uint8_t res = readBuffer[bufferIterator];
      bufferIterator++;
      fileReadIterator++;
      return res;
   }
   else
   {
        if(!drive->continueRead())
        {
           // больше ничего в файле нет
           fileReadIterator = fileSize;
        }
   }

  return 0;
    
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool USBFile::write(uint8_t* data, uint8_t dataLength)
{
  if(!openedFlag)
    return false;

  return drive->write(data,dataLength);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool USBFile::write(uint8_t value)
{
  uint8_t buff[1] = {value};
  return write(buff,1);   
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------


