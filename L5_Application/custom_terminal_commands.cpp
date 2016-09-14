/*
 * custom_handlers.cpp
 *
 * 	Contain custom terminal commands that the user can send through the Hercules program.
 *
 *  Created on: Sep 7, 2016
 *      Author: Derek Tran
 */




#include "command_handler.hpp"

#include "io.hpp"	// For Temperature IO access

/*
 * Echo temperature readings from the onboard temperature sensor.
 * Allow the user to specify the units and precision of the readout.
 *
 * In terminalTask::TaskEntry(), there should be the lines:
 *      
    CMD_HANDLER_FUNC(temperature_command_handler); // Prototype
    cp.addHandler(temperature_command_handler, "temp", "Help Description");
 */
CMD_HANDLER_FUNC(temperature_command_handler){

    if(cmdParams.getLen() < 1){
        output.printf("Expected argument. Type 'help temp' for more information.\n");
        return true;
    }

	int multiplier = 1;
    char operation = '\0';

    if(cmdParams.beginsWithIgnoreCase("f"))      operation = 'F';
    else if(cmdParams.beginsWithIgnoreCase("c")) operation = 'C';
    else if(cmdParams.beginsWithIgnoreCase("k")) operation = 'K';
    else {
        output.printf("Unknown temperature unit. Defaulting to F.\n");
    }

    cmdParams.eraseFirstWords(1);

		// Allow the user to specify the number of sigfigs to show after the decimal.
    unsigned short exponent = str::toInt(cmdParams);
	if(exponent > 0){
        unsigned short exp_cpy = exponent;
		while(exp_cpy-- > 0){
			multiplier *= 10;
		}

	} else {
		multiplier = 1000;
	}

    float floatTemp = 0.0;
    switch(operation){
        case 'C':
            floatTemp = TemperatureSensor::getInstance().getCelsius();
            break;
        case 'K':
            floatTemp = TemperatureSensor::getInstance().getCelsius() + 273;
            break;
        case 'F':
        default:
            operation = 'F';
            floatTemp = TemperatureSensor::getInstance().getFarenheit();
            break;
    }

    int floatSig1 = (int) floatTemp;
    int floatDec1 = ((floatTemp - floatSig1) * multiplier);	// Grab sigfigs

    str dec_cnv(exponent + 1);
    dec_cnv.printf("%u", floatDec1);
    while(dec_cnv.getLen() < exponent)  dec_cnv.insertAtBeg("0");
    output.printf("Current temperature: %u.%s %c\n", floatSig1, dec_cnv.c_str(), operation);

    return true;
}
