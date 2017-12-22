package com.nordicsemi.nrfUARTv2;

import android.location.Location;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * Created by Mats on 11/12/2017.
 */

public class Result {
    Location location;
    byte tag;
    List<String> UIDs = new ArrayList<String>();
    Boolean deadspot = true;

    public Result(Location location)
    {
        this.location = location;
    }


    public Result(Location location, byte tag, String UID)
    {
        this.location = location;
        this.tag = tag;
        UIDs.add(UID);
        deadspot = false;
    }

    public Result(Location location, byte tag)
    {
        this.location = location;
        this.tag = tag;
        deadspot = true;
    }

    public void addGateway(String UID)
    {
        UIDs.add(UID);
        deadspot = false;
    }

    public void setTag(byte tag){
        this.tag = tag;
    }

    public String toString(){
        if(!deadspot){
            String string = "UIDs: " + UIDs + "lat: " + location.getLatitude() + " long: " + location.getLongitude() + " tag: " + tag;
            return string;
        }
        else{
            String string = "Deadspot at: lat: " + location.getLatitude() + " long: " + location.getLongitude() + " tag: " + tag;
            return string;
        }

    }

    public JSONObject getJSONObject() {
        JSONObject obj = new JSONObject();
        try {
            obj.put("Tag", byteToHex(tag));
            obj.put("Dead spot", deadspot);
            JSONObject locJSON = new JSONObject();
            locJSON.put("Latitude", location.getLatitude());
            locJSON.put("Longitude", location.getLongitude());
            obj.put("Location", locJSON);

            JSONObject UIDJSON = new JSONObject();
            Iterator<String> iterator = UIDs.iterator();
            // while loop
            while (iterator.hasNext()) {
                UIDJSON.put("UID", iterator.next());
            }
            obj.put("Gateways", UIDJSON);

        } catch (JSONException e) {
        }
        return obj;
    }

    public static String byteToHex(byte b) {
        StringBuffer hexString = new StringBuffer();
        int intVal = b & 0xff;
        if (intVal < 0x10)
            hexString.append("0");
        hexString.append(Integer.toHexString(intVal));
        return hexString.toString();
    }

}
