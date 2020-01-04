#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#define CH376S_TIMEOUT 2000
#define CH376S_RETURN_OK 0x14
#define CH376S_BUFFER_SIZE 64

#define CH376_CMD_FRAME_START_1 0x57
#define CH376_CMD_FRAME_START_2 0xAB

#define CH376_CMD_SET_FILENAME 0x2F
#define CH376_CMD_DISK_MOUNT 0x31
#define CH376_CMD_IS_DISK_CONNECTED 0x30
#define CH376_CMD_RESET_ALL 0x05
#define CH376_CMD_CHECK_CONNECTION 0x06
#define CH376_CMD_USB_MODE 0x15
#define CH376_CMD_FILE_OPEN 0x32
#define CH376_CMD_FILE_CLOSE 0x36
#define CH376_CMD_FILE_REMOVE 0x35
#define CH376_CMD_FILE_CREATE 0x34
#define CH376_CMD_FILE_SIZE 0x0C
#define CH376_CMD_BYTE_READ 0x3A
#define CH376_CMD_RD_USB_DATA0 0x27
#define CH376_CMD_BYTE_RD_GO 0x3B
#define CH376_CMD_BYTE_WRITE 0x3C
#define CH376_CMD_WR_REQ_DATA 0x2D
#define CH376_CMD_BYTE_WR_GO 0x3D
#define CH376_CMD_BYTE_LOCATE 0x39
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class USBFile;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class CH376S_UART
{
  public:
    CH376S_UART(Stream& s);
    
    bool begin();
    
    bool isFlashDriveInserted();

    bool exists(const String& fileName);
    USBFile open(const String& fileName, bool createIfNotExists=false);
    bool remove(const String& fileName);


protected:

    friend class USBFile;
    
    bool setUSBMode(uint8_t value);
    
    bool waitForResponse(uint16_t timeout=CH376S_TIMEOUT);
    uint8_t getResponse();

    void setFileName(const String& fileName);
    
    bool doOpenFile(const String& fileName);
    bool doCloseFile(bool updateFileSize=false);
    
    bool doRemoveFile(const String& fileName);
    bool doCreateFile(const String& fileName);
    
    uint32_t getFileSize();
    
    bool startByteRead(uint8_t numBytes);
    uint8_t readBytes(uint8_t* buffer);    
    bool continueRead();
    
    bool doSeekSet(uint32_t pos);
    
    bool write(uint8_t* data, uint8_t dataLength);

private:

    Stream* workStream;
    
    void resetALL();
    bool checkConnection();
    bool mount();

    bool prepare();

    void sendStartFrame();

};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class USBFile
{
  public:
    USBFile(CH376S_UART* drive);
   ~USBFile();

    USBFile(const USBFile& rhs);
    USBFile& operator=(const USBFile& rhs);
  
    bool isOpen() { return openedFlag; }
    bool close(bool updateFileSize=false);
    
    uint32_t size() { return fileSize; }
    bool available();
    uint8_t read();
    bool write(uint8_t value);
    bool write(uint8_t* data, uint8_t dataLength);

    bool seekSet(uint32_t pos, bool atEnd=false);

  protected:
  
    friend class CH376S_UART;
    void open(const String& fileName, bool createIfNotExists=false);    

  private:
    bool openedFlag;
    uint32_t fileSize, fileReadIterator;
    int8_t bufferIterator;
    uint8_t readBuffer[CH376S_BUFFER_SIZE];
    uint8_t remain;
    void reset();

    CH376S_UART* drive;
  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------

