package com.nordicsemi.nrfUARTv2;

import android.location.Location;

import java.util.ArrayList;
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
}
