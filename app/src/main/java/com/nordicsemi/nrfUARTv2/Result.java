package com.nordicsemi.nrfUARTv2;

import android.location.Location;

/**
 * Created by Mats on 11/12/2017.
 */

public class Result {
    Location location;
    byte tag;

    public Result(Location location, byte tag)
    {
        this.location = location;
        this.tag = tag;
    }

    public String toString(){
        String string = "lat: " + location.getLatitude() + " long: " + location.getLongitude() + " tag: " + tag;
        return string;
    }
}
