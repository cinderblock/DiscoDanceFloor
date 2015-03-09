Disco Floor Node
================

Code for a single dance floor node. It handles sending and 
receiving message via RS485 to and from the master node.

For a simple test setup, you can make one of the nodes the "master"
by bringing pin 5 high. This node will then run the network through
a node registration and a series of simple programs.

Wiring
------

### Arduino

```
0:   To pin 1 of RS485 chip (RO)
1:   To pin 4 of RS485 chip (DI)
2:   To '-' for nodes, '+' for master
3:   Connect to pin 4 with 1M resistor
4:   To capacitive touch sensor area (eg, bare wire, sheet of aluminum foil, etc)
5:   To pin 6 of next node
6:   From pin 5 of the previous node or master
7:   To both TX/RX enable pins of RS485 chip (pins 2 & 3)
9:   To green pin of RGB LED
10:  To blue pin of RGB LED
11:  To red pin of RGB LED
12:  (master only, optional) Attach to pin 13 of either node to receive debugging statements
13:  (optional) Attach to master's pin 12, for debugging statements.
```


### RS485 chip (MAX485 or ISL8487E)

I'm using the [Intersil ISL8487E](http://www.digikey.com/product-detail/en/ISL8487EIBZ/ISL8487EIBZ-ND/1034816) chip 
which is pin compatible with MAS485, but cheaper. Both pins 2 and 3 (RE & DE) can be wired together and then connected
to pin 7 of the floor node.

```
1:   To pin 0 (RX) of floor node
2:   To pin 7 of floor node
3:   To pin 7 of floor node
4:   To pin 1 (TX) of floor node
5:   To 5V
6:   Bus A/Y
7:   Bus B/Z
8:   To common
```

Test Master Flow
----------------

1. Enable first floor node (pin 5 HIGH)
2. Ask for the node's address (repeat message after ACK_TIMEOUT)
3. When node responds with address, respond with ACK. Node will then enable the next node.
4. Repeat steps 3 & 4 until no more nodes respond for 5 seconds (defined by ADDRESSING_TIMEOUT)
5. Get status of all nodes
  1. Send global (*) request for node status.
  2. Each node responds with current state: current color, is fading (1 or 0), and if sensor detects person (1 or 0)
  3. If the repsonse chain fails for some reason (i.e. node 3 doesn't respond), master sends another status
     request for all nodes from that node forward (3-*).
  4. If still no status response from the same node, move to the next node forward (4-*)
6. Run programs, switching every 10 seconds (defined by PROGRAM_TIMEOUT):
  1. Color program: Sets ALL LEDs to a single solid color.
  2. Differnet color program: Sets all LEDs to different solid colors.
  3. Fade program: Fade all LEDs to different colors.
  4. Repeat


Debugging
---------
All nodes output serial debugging statements to pin 13 (4800 baud). If pin 12 of the master node is
connected to this pin, it will dump these statements out to serial.
