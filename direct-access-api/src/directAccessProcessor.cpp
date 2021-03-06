/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 * *****************************************************************************
 */

#include "directAccessProcessor.hpp"
#include <iostream>
#include <sstream>
#include <stdint.h>
#include "server_ws.hpp"
#include "visconf.hpp"
#include "exception.hpp"


using namespace std;

directAccessProcessor::directAccessProcessor(class  authenticator* vdator) {
   tokenValidator = vdator;
   accessValidator = new accesschecker(tokenValidator); 
}


string directAccessProcessor::processCreate(class wschannel& channel, string rchannel) {

#ifdef DEBUG
  // cout<< "Create :: path received from client = "<< rchannel <<endl;
#endif
    
  // TODO- create VCAN here .... Now returning dummy value. 

   if( channel.accesses.has_key(rchannel) ) {
   json res;
   res["action"] = "create";
   res["vcan"] = "VCAN0";
   res["rcan"] = rchannel;
   res["timestamp"]= time(NULL);
   stringstream ss; 
   ss << pretty_print(res);
   cout << " Created VCAN Channel " << res["vcan"].as<string>() << " for Real CAN " << rchannel << " with access on : " << channel .accesses << endl;
   return ss.str();
  } else { 
    json result;
           json error;
	   result["action"] ="create";
           
           error["reason"] = "not authorized to create " + rchannel ;
          

	   result["error"] = error;
	   result["timestamp"]= time(NULL);
	
	   std::stringstream ss;
   	   ss << pretty_print(result);
	   return ss.str();


  }
}


string directAccessProcessor::processDelete(string vchannel) {

   json answer;
   answer["action"] ="delete";
   answer["timestamp"] = time(NULL);
   
   std::stringstream ss;
   ss << pretty_print(answer);
   return ss.str();    
}


string directAccessProcessor::processAuthorize (string token ,class wschannel& channel) {


        int ttl = tokenValidator->validate(channel, token);

        if( ttl == -1) {
           json result;
           json error;
	   result["action"] ="authorize";
           error["number"] = 401;
           error["reason"] = "Invalid Token";
           error["message"] = "Check the JWT token passed";

	   result["error"] = error;
	   result["timestamp"]= time(NULL);
	
	   std::stringstream ss;
   	   ss << pretty_print(result);
	   return ss.str();

        } else {

           json result;
	   result["action"] ="authorize";
           result["TTL"]= ttl;
	   result["timestamp"]= time(NULL);
	
	   std::stringstream ss;
   	   ss << pretty_print(result);
	   return ss.str();
        } 

}


string directAccessProcessor::processQuery(string req_json , class wschannel& channel) {

	json root;
        string response;
        root = json::parse(req_json);
	string action = root["action"].as<string>();
	
        if ( action == "authorize") {
                string token = root["tokens"].as<string>();
                
		//cout << "vsscommandprocessor::processQuery: authorize query with token = " << token << endl;
                response = processAuthorize (token , channel); 
        } else {
                string can_channel = root["channel"].as<string>();
                
                bool hasAccess = accessValidator->checkAccess(channel , can_channel);
                
                if (!hasAccess) {
                    json result;
                    json error;
	   	    result["action"] =action;
           	    error["number"] = 403;
                    error["reason"] = "Forbidden";
                    error["message"] = "Not authorized to access resource";

	            result["error"] = error;
	   	    result["timestamp"]= time(NULL);
	
	            std::stringstream ss;
   	            ss << pretty_print(result);
	            response = ss.str();

                } else if (action == "create")  { 
		  // cout << "vsscommandprocessor::processQuery: create query  for " << can_channel << endl;
                   response = processCreate(channel, can_channel);
	        } else if(action == "delete")  {
		  // cout << "vsscommandprocessor::processQuery: delete query  for " << can_channel << endl;
		   response = processDelete(can_channel);
       	        } else {
		  // cout << "vsscommandprocessor::processQuery: Unknown action " << action << endl;
	        }
        }

   return response;

}


