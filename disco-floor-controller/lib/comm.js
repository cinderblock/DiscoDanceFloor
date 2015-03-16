/**

	The communication interface between the dance floor
	and the RS485 serial bus.

*/

'use strict';

var EventEmitter  = require("events").EventEmitter,
		util   			  = require("util"),
		Serial 			  = require('serialport').SerialPort,
		MessageParser = require('./serial_message_parser.js'),
		discoCntrl    = require('./disco_controller.js').controller,
		discoUtils 		= require('./utils.js'),
		_             = require('underscore');

const BAUD_RATE			      = 9600;
const ACK_TIMEOUT         = 100;
const STATUS_TIMEOUT      = 500;
const ADDRESSING_TIMEOUT  = 1000;

// Program stages
const IDLE                = 0x00;
const ADDRESSING          = 0x01;
const STATUSING           = 0x02;
const UPDATING            = 0x03;

// Status flags
const FADING           		= 0x01;
const SENSOR_DETECT    		= 0x02;

var serialPort,
		txBuffer,
		rxBuffer,
		stage = IDLE,
		statuses = {},
		lastStatusAddr,
		statusTimeout,
		statusTries,
		lastNodeAddr = MessageParser.MASTER_ADDRESS,
		lastUpdate = 0,
		addressingStageTimeout,
		nodeRegistration;


// Set our address on the parser
MessageParser.setMyAddress(MessageParser.MASTER_ADDRESS);


/**
	Handles communication with the floor nodes
	over the serial port

	@inherits EventEmitter
*/
function Comm(){
	EventEmitter.call(this);
}
// util.inherits(Comm, EventEmitter);
Comm.prototype = {

	/**
		Start communicating with all the floor cells
		over the serial port

		@param {String} port The serial port to the RS485 bus
		@return SerialPort
	*/
	start: function (port){
		nodeRegistration = [];

		serialPort = new Serial(port, {
			baudrate: BAUD_RATE,
			parser: serialParser
		});

		// Open and process incoming data
		serialPort.on('open', function () {
			MessageParser.setSerialPort(serialPort);

			// Listen for new messages
			serialPort.on('message-ready', function(message) {

				// Only message that we have not sent
				if (message.srcAddress != MessageParser.MASTER_ADDRESS) {
					// console.log('Message received: ' + message.getTypeAsString(),
					// 						'From:', message.normalizeAddress(message.srcAddress));
					this.handleMessage(message);
				}

			}.bind(this));

			// Start node address registration
			txBuffer = null;
			this.nextStage();
		}.bind(this));

		return serialPort;
	},

	/**
		Move onto the next stage:

			1. ADDRESSING
			2. STATUSING
			3. UPDATING
			4. Repeat 2 - 4

		@method nextStage
	*/
	nextStage: function() {
		if (txBuffer) {
			txBuffer.reset();
			txBuffer = null;
		}

		// From the old status to the new status
		switch(stage) {
			case IDLE:
				stage = ADDRESSING;
			break;
			case ADDRESSING:
				if (nodeRegistration.length) {
					stage = STATUSING;
					this.emit('done-addressing', nodeRegistration.length);
				}
				// Nothing found, continue
				else {
					this.addressing();
				}
			break;
			case STATUSING:
				stage = UPDATING;
			break;
			case UPDATING:
				stage = STATUSING;
			break;
		}

		// Setup and call the new status handler on the next tick of the event loop
		switch(stage) {
			case ADDRESSING:
				process.nextTick(this.addressing.bind(this));
			break;
			case STATUSING:
				// console.log('STATUSING');
				lastStatusAddr = MessageParser.MASTER_ADDRESS;
				process.nextTick(this.status.bind(this));
				// setTimeout(this.status.bind(this), 10);
			break;
			case UPDATING:
				// console.log('UPDATING');
				process.nextTick(this.update.bind(this));
				// setTimeout(this.update.bind(this), 10);
			break;
		}
	},

	/**
		Pass the most recent message to the correct stage.
		For example, all messages received during the addressing stage
		will be sent to the `addressing` method.
	*/
	handleMessage: function(message) {
		switch(stage) {
			case ADDRESSING:
				this.addressing(message);
			break;
			case STATUSING:
				this.status(message);
			break;
		}
	},


	/**
		Handle the addressing stage

		@method addressing
		@param {MessageParser} message (optional) The most recent message recieved
	*/
	addressing: function(message) {
		var addr;

		// Start sending address requests
		if (!txBuffer) {
			txBuffer = sendAddressingRequest();
			addressingStageTimeout = setTimeout(this.nextStage.bind(this), ADDRESSING_TIMEOUT);
		}

		// Register new address
		else if (message && message.type == MessageParser.TYPE_ADDR) {
			addr = message.getMessageBody()[0];

			// New address must be larger than the last one added
			if (addr > lastNodeAddr) {
				txBuffer.stopSending();

				nodeRegistration.push(addr);
				discoCntrl.addCellWithAddress(addr);
				console.log('Add node at address', addr);

				lastNodeAddr = addr;
				this.emit('new-node', addr);

				// Send ACK, and then query for the next address
				sendACK(addr)
				.then(function(){
					txBuffer = sendAddressingRequest();
				});

				// Update timeout
				clearTimeout(addressingStageTimeout);
				addressingStageTimeout = setTimeout(this.nextStage.bind(this), ADDRESSING_TIMEOUT);
			}
			else {
				console.log('Invalid address:', addr);
			}
		}
	},

	/**
		Handle the status stage

		@method status
		@param {MessageParser} message (optional) The most recent message recieved while in this stage
	*/
	status: function(message) {
		var sensor, addr;

		// All statuses received, move on
		if (lastStatusAddr == lastNodeAddr) {
			this.nextStage();
		}

		// Register status
		else if (message && message.type == MessageParser.TYPE_STATUS) {
			sensor = message.getMessageBody()[0] & SENSOR_DETECT;
			addr = message.srcAddress;

			if (addr > lastStatusAddr) {

				statusTries = 0;
				lastStatusAddr = addr;
				statuses[addr] = message.getMessageBody();

				// Update retry timeout
				clearTimeout(statusTimeout);
				statusTimeout = setTimeout(sendStatusRequest, STATUS_TIMEOUT);
			}
		}

		// Send status request
		else if (!txBuffer) {
			statusTries = 0;
			txBuffer = sendStatusRequest();
		}
	},

	/**
		Handle the node update stage
	*/
	update: function() {
		nodeRegistration.forEach(function(addr){
			var cell = discoCntrl.getCellByAddress(addr),
					status = statuses[addr];

			// Could not find cell or status
			if (!cell || !status) {
				console.log('No cell or status for', addr, cell, status);
				return;
			}

			// Process status and sync
			if (!processStatus(addr, cell, status)) {
				sendFloorCell(addr, cell);
			}
		});

		// // Set random color fades every 1.5 seconds
		// if (lastUpdate + 1500 < Date.now()) {
		// 	var data = [0,0,0,4], // 8 = 1 second fade (sent as number of 250ms increments)
		// 			maxValue = 120,
		// 			rgbSelect, tx;


		// 	// Each node
		// 	nodeRegistration.forEach(function(addr){
		// 		tx = new MessageParser();
		// 		data[0] = 0;
		// 		data[1] = 0;
		// 		data[2] = 0;

		// 		// Set a two colors to fade to
		// 		// (first can go from 0 - 120, secondary can go from 0 - 255)
		// 		for (var c = 0; c < 2; c++) {
		// 			rgbSelect = Math.floor(Math.random() * 3); // Which RGB color to set
		// 			if (c == 1) maxValue =	255;
		// 			data[rgbSelect] = Math.floor(Math.random() * maxValue);
		// 		}

		// 		tx.start(MessageParser.TYPE_FADE);
		// 		tx.setDestAddress(addr);
		// 		tx.write(data);
		// 		tx.send();
		// 	});

		// 	lastUpdate = Date.now();
		// }

		this.nextStage();
	}

};
Comm.prototype.__proto__ = EventEmitter.prototype;
var comm = new Comm();

/**
	A custom SerialPort parser that runs incoming data through the
	MessageParser and emits `message-ready` every time it finds
	an incoming message.

	@function serialParser
	@params {Emitter} emitter The SerialPort event emitter
	@params {Buffer} buffer Incoming data
*/
function serialParser(emitter, buffer) {
	if (!rxBuffer) {
		rxBuffer = new MessageParser();
	}

	for (var i = 0; i < buffer.length; i++){
		rxBuffer.parse(buffer.readUInt8(i));

		if (rxBuffer.isReady()){
			emitter.emit('message-ready', rxBuffer);
			rxBuffer = new MessageParser();
		} else {
			emitter.emit('data', buffer[i]);
		}
	}
}

/**
	Send the last node address to the bus during
	the address registration stage.

	@function sendAddressingRequest
	@return {MessageParser} The sent message object
*/
function sendAddressingRequest() {
	var tx = new MessageParser();

	tx.start(MessageParser.TYPE_ADDR);
	tx.setDestAddress(MessageParser.MSG_ALL);
	tx.write(lastNodeAddr);
	tx.sendEvery(ACK_TIMEOUT);

	return tx;
}

/**
	Send a request for node status

	@function sendStatusRequest
	@return {MessageParser or False} The sent message object or `False` if there are not more nodes
*/
function sendStatusRequest() {
	var tx = new MessageParser();

	// If we've failed to get status at least twice, skip this node
  if (statusTries >= 2) {
    console.log('No status received from', lastStatusAddr + 1, 'moving on');
    lastStatusAddr++;
    statusTries = 0;
  }

  // We're out of nodes
  if (lastStatusAddr + 1 >= lastNodeAddr) {
  	comm.nextStage();
    return false;
  }

  tx.start(MessageParser.TYPE_STATUS);
  tx.setDestAddress(lastStatusAddr + 1, MessageParser.MSG_ALL);
  tx.send();
  statusTries++;

  // Automatically attempt again
  statusTimeout = setTimeout(sendStatusRequest, STATUS_TIMEOUT);

  return tx;
}

/**
	Sends the values from FloorCell to the physical cell node

	@private
	@function sendFloorCell
	@param {byte} addr The address of the node to update
	@param {FloorCell} cell The object to get the cell state from
	@return Promise
*/
function sendFloorCell(addr, cell) {
	var tx = new MessageParser(),
			type = (cell.isFading()) ? MessageParser.TYPE_FADE : MessageParser.TYPE_COLOR,
			data = cell.getColor();

	if (cell.isFading()) {
		type = MessageParser.TYPE_FADE;
		data = cell.getFadeColor().slice(0);
		data.push(Math.round(cell.getFadeDuration() / 250));
	}

	tx.start(type);
	tx.setDestAddress(addr);
	tx.write(data);
	return tx.send();
}

/**
	Process the status received from on of the floor
	nodes and sync it with the FloorCell that represents it.

	Here's how it work:
		* The FloorCell is the source of truth for the color & fading state of the node
		* If the node color and the FloorCell color do not match, update the node
		* If the fading status between the node and the Floor cell are different:
		    + If the node's color is the same as the FloorCell's target color, tell the FloorCell the fade is complete
		    + Otherwise, sync the node
    * Update the FloorCell value to match the node's sensor value -- only if it has changed;

	Does the FloorCell object match the status of
	what was returned from the bus

	@private
	@function processStatus
	@param {byte} addr The node address
	@param {FloorCell} cell
	@param {Array of bytes} status
	@return {boolean} True if the node and FloorCell are in sync
*/
function processStatus(addr, cell, status) {
	var hasFadeFlag = (status[0] & FADING),
			sensorDetect = (status[0] & SENSOR_DETECT) ? 1 : 0,
			color = cell.getColor(),
			statusColor = status.slice(1, 4),
			targetColor = (cell.isFading()) ? cell.getFadeColor() : color,
			statusTargetColor = (hasFadeFlag) ? status.slice(4, 7) : statusColor;

	// Set FloorCell value to match sensor value
	if (sensorDetect != cell.getValue()) {
		console.log('Detected change!');
		cell.setValue(sensorDetect ? 1 : 0);
	}

	// No longer fading
	if (hasFadeFlag && !cell.isFading()) {
		console.log('Fading mismatch', cell.isFading(), status[0]);
		return false;
	}

	// Colors mismatch
	else if (!_.isEqual(targetColor, statusTargetColor)) {
		console.log('Color mismatch', targetColor, statusTargetColor);
		return false;
	}

	// Update color for fade step from the floor
	if (cell.isFading() && !_.isEqual(color, statusColor)) {
		cell.setColor(statusColor, !hasFadeFlag);
	}

	return true;
}

/**
	Send an ACK message to a node address

	@function sendACK
	@param {byte} addr The destination address
	@return {Promise}
*/
function sendACK(addr) {
	var tx = new MessageParser();
	tx.start(MessageParser.TYPE_ACK);
  tx.setDestAddress(addr);
  return tx.send();
}

module.exports = new Comm();