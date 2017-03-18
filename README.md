# Soil Moisture Sensor SMD
##  


# What is this ?

Simple office plant monitor inspired by [@carlierd](https://www.openhardware.io/user/97/projects/carlierd) on the [this](https://www.openhardware.io/view/123/Office-plant-monitor) post.

This is my first SMD project, i hope he'll used by other members ;)

The skech is 2.0.0 Mysensors compatible, i haven't test this sketch with the 2.1.1 version.

![Soil Moisture Sensor SMD](https://www.openhardware.io//uploads/58a09467db20b9ab3c6b1055/image/IMG_6587.JPG "Soil Moisture Sensor SMD")

The PCB files are for the V1.1.2 version, i haven't posted the V1.1 because there were important mistakes on the v1.1.

I have put 3 leds on this project :

* one connected on SCK for see the same thing than an arduino pro mini
* one for see when the node is initialised and when is goes in a sleep mode
* one not used for the moment, my idea was to use this led for offline node but i don't know how to do this without alterate the autonomie of the battery :(

# Features :

* Send Soil moisture mesure every hours
* Send Battery Level every hours if the value has changed
* Send Battery voltage every hours if the percentage has changed

# Compatibilies :

* Mysensors Library 2.0.0
* PCBs v1.1.x compatibles with Soil_Moisture_Sensor_SMD_v1.1.x sketchs

# Logs PCB :
## V1.1.2

* 16/02/2017 = Fabrication V1.1.2 by PCBs.io => Awaiting Panelization
* 20/02/2017 = Fabrication V1.1.2 by PCBs.io => In Fabrication
* 27/02/2017 = Fabrication V1.1.2 by PCBs.io => Shipped
* 09/03/2017 = Fabrication V1.1.2 by PCBs.io => Received
* 11/03/2017 = Assembly and Test => Functional Board but some minors issues => see v1.1.2a

## V1.1.2a
### minors modifications

* Increase Drill battery older CR123
* Modify text "FTDI" by "AVRISP"
* Add text "FTDI"

# Logs Sketch:
## V1.1.1

* Add default values
* Do not send moisture to the controler if she didn't change more than 1%
* Change Low Fuse example

