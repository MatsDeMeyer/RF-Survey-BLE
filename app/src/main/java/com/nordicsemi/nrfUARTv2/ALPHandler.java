package com.nordicsemi.nrfUARTv2;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Random;

/**
 * Created by Mats on 2/12/2017.
 */

public class ALPHandler {
    List<Integer> unhandledIndices = new ArrayList<Integer>();
    List<Byte> unhandledTags = new ArrayList<Byte>();
    List<Result> OkResults = new ArrayList<Result>();
    List<Result> NokResults = new ArrayList<Result>();

    public byte[] GenerateALP(){

        //41 54 24 44 c0 00 0c b4 37 32 d7 01 00 10 01 01 00 00 08

        Random random = new Random();

        //initialize variable with first four bytes (AT$0xd7)
        byte[] command = {(byte)0x41, (byte)0x54, (byte)0x24, (byte)0x44};

        //append serial header (third byte is length, will be determined later)
        byte[] serial = {(byte)0xc0, (byte)0x00, (byte)0x00};
        command = concat(command, serial);

        //append ALP command
        //generate random tag
        byte[] TAG = new byte[1];
        random.nextBytes(TAG);
        unhandledTags.add(TAG[0]);
        //add to NOK list
        NokResults.add(new Result(MainActivity.currentLocation, TAG[0]));
        unhandledIndices.add(NokResults.size()-1);

        byte[] ALP = {  (byte)0xb4, //tag response action
                        TAG[0], //tag
                        (byte)0x32,
                        (byte)0xd7,
                        (byte)0x01,
                        (byte)0x00,
                        (byte)0x10,
                        (byte)0x01,
                        (byte)0x01,
                        (byte)0x00, //firmware file (uid)
                        (byte)0x00,
                        (byte)0x08};
        command = concat(command, ALP);

        //set serial length byte to length of ALP command
        command[6] = (byte)ALP.length;

        return command;
    }

    public String ParseALP(byte[] response){
        /*
        Response op readfiledata: c0 00 07 20 00 00 01 42 a3 55
        serial: c0 00 07 (length)
        action: 20
        file id: 0
        offset: 0
        size: 1
        file data: 42
        ??: a3 (163)
        tag: 55


        met python: c0 00 24 62 d7 38 00 00 1f 29 50 00 20 00 00 20 01 43 37 31 34 00 3e 00 41 20 00 00 08 43 37 31 34 00 3e 00 41 23 37
                    c0 00 02 a3 37

        met app:    c0 00 24 62 d7 38 00 00 1d 27 50 00 6c
       (per 1 byte) c0 00 02 a3 37

                2:  c0 00 24 62 d7 38 00 00 18 22 50 00 c2 00 00 20 01 43 37 31 34 00 3e
                    c0 00 02 a3 37
                3:  c0 00 24 62 d7 38 00 00 17 21 50 00 de 00 00 20 01 43 37 31 34 00 3e 00 41 20 00 00 08 43 37 31 34
                    c0 00 02 a3 37
                4:  c0 00 24 62 d7 38 00 00 1c 26 50 00 21 00 00 20 01 43 37 31 34 00 3e 00 41 20 00 00 08 43 37 31 34 00 3e 00 41 23 37
                    c0 00 02 a3 37

         c0 komt niet altijd door?
         response pas na volgende ALP command

         */
        if(unhandledTags.contains(response[response.length-1]))
        {
            System.out.println("Ok Results: " + OkResults.toString());
            System.out.println("Nok Results: " + NokResults.toString());

            Result parsedResult = NokResults.get(unhandledIndices.get(unhandledIndices.size()-1));
            NokResults.remove(NokResults.get(unhandledIndices.get(unhandledIndices.size()-1)));
            OkResults.add(parsedResult);
            unhandledTags.remove(unhandledTags.indexOf(response[response.length-1]));

            System.out.println("Ok Results: " + OkResults.toString());
            System.out.println("Nok Results: " + NokResults.toString());
            return "Response OK, remaining unhandled tags: " + unhandledTags.size();
        }
        else
            return "Reponse not OK";
    }

    public String results(){
        return "Ok/Nok Results: " + OkResults.size() + "/" + NokResults.size();
    }

    byte[] concat(byte[]...arrays)
    {
        // Determine the length of the result array
        int totalLength = 0;
        for (int i = 0; i < arrays.length; i++)
        {
            totalLength += arrays[i].length;
        }

        // create the result array
        byte[] result = new byte[totalLength];

        // copy the source arrays into the result array
        int currentIndex = 0;
        for (int i = 0; i < arrays.length; i++)
        {
            System.arraycopy(arrays[i], 0, result, currentIndex, arrays[i].length);
            currentIndex += arrays[i].length;
        }

        return result;
    }

    //JSON ipv csv? array per tag
    public static final String CSV_SEPARATOR = ",";
    public void writeToCSV()
    {
        try
        {
            String baseDir = android.os.Environment.getExternalStorageDirectory().getAbsolutePath();
            String currentDateString = DateFormat.getDateInstance().format(new Date());
            String currentTimeString = DateFormat.getTimeInstance().format(new Date());
            String fileName = "RF-Survey-Result " + currentDateString + "-" + currentTimeString + ".csv";
            String filePath = baseDir + File.separator + fileName;
            BufferedWriter bw = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(filePath), "UTF-8"));

            String header = "Status" + CSV_SEPARATOR + "Tag (hex)" + CSV_SEPARATOR + "Latitude" + CSV_SEPARATOR + "Longitude";
            bw.write(header);
            bw.newLine();

            for (Result result : OkResults)
            {
                String oneLine = "OK" + CSV_SEPARATOR + Integer.toHexString(result.tag & 0xFF) +
                        CSV_SEPARATOR +
                        result.location.getLatitude() +
                        CSV_SEPARATOR +
                        (result.location.getLongitude());
                bw.write(oneLine);
                bw.newLine();
            }

            for (Result result : NokResults)
            {
                String oneLine = "Dead Spot" + CSV_SEPARATOR + Integer.toHexString(result.tag & 0xFF) +
                        CSV_SEPARATOR +
                        result.location.getLatitude() +
                        CSV_SEPARATOR +
                        result.location.getLongitude();
                bw.write(oneLine);
                bw.newLine();
            }

            bw.flush();
            bw.close();
        }
        catch (UnsupportedEncodingException e) {} catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    public void discardResults()
    {
        OkResults = new ArrayList<Result>();
        NokResults = new ArrayList<Result>();
        unhandledIndices = new ArrayList<Integer>();
        unhandledTags = new ArrayList<Byte>();
    }



}


