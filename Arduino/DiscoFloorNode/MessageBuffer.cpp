
#include "MessageBuffer.h"

MessageBuffer::MessageBuffer(uint8_t txCntl) {
  myAddress = 0;
  txControl = txCntl;
  reset();
}

void MessageBuffer::start(uint8_t messageType) {
  type = messageType;

  if (type == 0) {
    messageState = MSG_STATE_IDL;
  } else {
    messageState = MSG_STATE_ACT;
  }

  sentAt = 0;
  // escaped = true;
  bufferPos = 0;
  isNew = true;

  srcAddress = 0;
  addressDestRange[0] = 0;
  addressDestRange[1] = 0;
}
void MessageBuffer::reset() {
  start(0);
}

uint8_t MessageBuffer::getType() {
  return type;
}

uint8_t MessageBuffer::getState() {
  return messageState;
}

void MessageBuffer::setMyAddress(uint8_t addr) {
  myAddress = addr;
}

uint8_t MessageBuffer::getSourceAddress() {
  return srcAddress;
}

void MessageBuffer::setDestAddress(uint8_t start, uint8_t end) {
  messageState = MSG_STATE_ACT;
  addressDestRange[0] = start;
  addressDestRange[1] = end;
}
void MessageBuffer::setDestAddress(uint8_t addr) {
  setDestAddress(addr, addr);
}

uint8_t MessageBuffer::getLowerDestRange() {
  return addressDestRange[0];
}

uint8_t MessageBuffer::getUpperDestRange() {
  return addressDestRange[1];
}

bool MessageBuffer::addressedToMe() {

  // Wildcard
  if (addressDestRange[0] == MSG_ALL) return true;

  // We have not set our address
  if (myAddress == 0) return false;

  // Match range
  if (addressDestRange[1] == MSG_ALL && addressDestRange[0] >= myAddress) return true;
  if (addressDestRange[0] >= myAddress && addressDestRange[0] <= myAddress) return true;

  return false;
}

bool MessageBuffer::addressedToMaster() {
  return addressDestRange[0] == MASTER_ADDRESS;
}

bool MessageBuffer::isReady() {
  return messageState == MSG_STATE_RDY;
}

uint8_t* MessageBuffer::getBody() {
  return buffer;
}

int MessageBuffer::getBodyLen() {
  return bufferPos;
}

uint8_t MessageBuffer::addToBuffer(uint8_t c) {
  if (messageState >= MSG_STATE_RDY) return messageState;

  buffer[bufferPos++] = c;
  buffer[bufferPos]   = '\0'; // Null terminator

  // Buffer over flow
  if (bufferPos == MSG_BUFFER_LEN - 1) {
    messageState = MSG_STATE_BOF;
  }
  return messageState;
}

uint8_t MessageBuffer::processHeader() {
  uint8_t headerPos = 0,
          destination;

  // No header or header is too long, corrupt message
  if (bufferPos == 0 || bufferPos > 5) {
    return messageState = MSG_STATE_ABT;
  }

  // Get destination address
  destination = buffer[headerPos++];

  // Address range
  if (buffer[headerPos] == '-') {
    setDestAddress(destination, buffer[++headerPos]);
    headerPos++;
  } else {
    setDestAddress(destination);
  }

  // Source address
  if (headerPos < bufferPos) {
    srcAddress = buffer[headerPos];
  }
  else {
    srcAddress = MASTER_ADDRESS;
  }

  // Is not addressed to us or MASTER
  // if (!addressedToMe() && !addressedToMaster()) {
  //   return messageState = MSG_STATE_ABT;
  // }

  // Reset buffer and prepare to process message
  bufferPos = 0;
  return messageState = MSG_STATE_ACT;
}

uint8_t MessageBuffer::write(uint8_t c) { 

  // Escape characer
  // if (escaped) {
  //   if(messageState == MSG_STATE_ACT) {
  //     addToBuffer(c);
  //   } 
  //   escaped = false;
  //   return messageState;
  // }
  // else if (c == MSG_ESC) {
  //   escaped = true;
  // }

  // Start of message
  if(c == MSG_SOM) {
    reset();
    messageState = MSG_STATE_HDR;
  }

  // Aborted or overflow, wait until we see a new message
  else if (messageState >= MSG_STATE_RDY) {
    return messageState;
  }

  // End of message
  else if (c == MSG_EOM) {
    if(messageState == MSG_STATE_ACT) {
      isNew = true;

      // Compare checksum
      uint8_t checksum = buffer[--bufferPos];
      buffer[bufferPos] = '\0';
      if (calculateChecksum() != checksum) {
        Serial.print(F("CHECKSUMS MISMATCH: "));
        Serial.print(checksum); Serial.print(" != "); Serial.println(calculateChecksum());
        return messageState = MSG_STATE_ABT;
      }

      return messageState = MSG_STATE_RDY;
    } else {
      reset();
    }
  }

  // Header
  else if (messageState == MSG_STATE_HDR) {
    if (c == MSG_EOH) return processHeader();
    return addToBuffer(c);
  }

  // Message body
  else if(messageState == MSG_STATE_ACT) {

    // First character is message type
    if (type == 0 && bufferPos == 0) {
      type = c;
    } else {
      return addToBuffer(c);
    }
  }

  return messageState;
}


// Calculate the checksum for this message
uint8_t MessageBuffer::calculateChecksum() {
  uint8_t checksum = 0;
  if (messageState != MSG_STATE_RDY && messageState != MSG_STATE_ACT) return 0;

  checksum = _crc_ibutton_update(checksum, addressDestRange[0]);
  checksum = _crc_ibutton_update(checksum, addressDestRange[1]);
  checksum = _crc_ibutton_update(checksum, srcAddress);
  checksum = _crc_ibutton_update(checksum, type);
  for(int i = 0; i < bufferPos; i++ ){
    checksum = _crc_ibutton_update(checksum, buffer[i]);
  }

  return checksum;
}

uint8_t MessageBuffer::write(uint8_t* buf, uint8_t len) {
  for(int i = 0; i < len; i++) {
    write(buf[i]);
  }
  return messageState;
}

uint8_t MessageBuffer::read() {
  digitalWrite(txControl, RS485Receive);
  while (Serial.available() > 0) {
    write((uint8_t)Serial.read());
  }
  return messageState; 
}

uint8_t MessageBuffer::send() {
  uint8_t sent = 0,
          checksum;

  if (messageState != MSG_STATE_RDY && messageState != MSG_STATE_ACT) return 0;
  if (myAddress == 0) return 0;

  // Start sending
  srcAddress = myAddress;
  digitalWrite(txControl, RS485Transmit);

  Serial.print(MSG_SOM); sent++;

  // Destination address
  Serial.write(addressDestRange[0]); sent++;
  if (addressDestRange[0] != addressDestRange[1]) {
    Serial.write('-');
    Serial.write(addressDestRange[1]); 
    sent += 2;
  }

  // From address
  if (srcAddress != MASTER_ADDRESS) {
    Serial.write(srcAddress);
    sent++;
  }

  // Type and message body
  Serial.print(MSG_EOH);
  Serial.write(type);

  sent += 2;
  for(int i = 0; i < bufferPos; i++ ){
    Serial.write(buffer[i]);
  }
  sent += bufferPos - 1;

  // End of message
  checksum = calculateChecksum();
  Serial.write(checksum);
  Serial.print(MSG_EOM);
  sent += 2;
  Serial.flush();
  sentAt = millis();

  // Set back to receive
  digitalWrite(txControl, RS485Receive);

  return sent;
}
