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
    //List<Integer> unhandledIndices = new ArrayList<Integer>();
    List<Byte> unhandledTags = new ArrayList<Byte>();
    List<Byte> handledTags = new ArrayList<Byte>();
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
         BUG MET RESPONSE ORDER? TAGS?
         response pas na volgende ALP command?
         */

        if(response[response.length - 2] == (byte)0xe3 && response[response.length - 3] == (byte)0x02 && response[response.length - 4] == (byte)0x00)
        {
            byte tag = response[response.length-1];
            unhandledTags.remove(unhandledTags.indexOf(tag));
            handledTags.add(tag);
            NokResults.add(new Result(MainActivity.currentLocation, tag));
            System.out.println("Ok Results: " + OkResults.toString());
            System.out.println("Nok Results: " + NokResults.toString());

            return "No Gateways found";
        }
        else if(response[response.length - 2] == (byte)0xa3 && response[response.length - 3] == (byte)0x02 && response[response.length - 4] == (byte)0x00)
        {
            System.out.println("Response: " + MainActivity.byteArrayToHexString(response));

            byte ALPCommands[] = Arrays.copyOfRange(response, 0, response.length-5);
            System.out.println("ALP before: " + MainActivity.byteArrayToHexString(ALPCommands));

            if(ALPCommands[0] == (byte)0x00)
            {
                //add 0xc0 to the front (this gets cut off sometimes
                byte[] c0Array = {(byte)0xc0};
                ALPCommands = concat(c0Array, ALPCommands);
            }
            System.out.println("ALP fixed: " + MainActivity.byteArrayToHexString(ALPCommands));

            /*
            Testing multiple gateways:
            ALPCommands = concat(ALPCommands, ALPCommands);


            System.out.println("ALP twice: " + MainActivity.byteArrayToHexString(ALPCommands));
            */

            //create result variable
            Result result = new Result(MainActivity.currentLocation);
            //last byte of ALP command is tag
            byte tag = ALPCommands[ALPCommands.length-1];
            //update tag in results
            result.setTag(tag);
            if(unhandledTags.indexOf(tag) != -1)
            {
                unhandledTags.remove(unhandledTags.indexOf(tag));
            }
            handledTags.add(tag);

            //cut array into chunks of 39 (chunk per gateway)
            byte[][] ALPCommand = splitBytes(ALPCommands, 39);
            System.out.println("Chunks found: " + ALPCommand.length);
            for(int j = 0; j < ALPCommand.length; j ++)
            {
                //cut out UID part (8 bytes)
                byte UIDPart[] = Arrays.copyOfRange(ALPCommand[j], 17, 25);
                String UID = MainActivity.byteArrayToHexString(UIDPart);
                //add UID to UID array in result
                result.addGateway(UID);
                System.out.println("Gateway added: " + UID);
            }
            OkResults.add(result);
            System.out.println("Ok Results: " + OkResults.toString());
            System.out.println("Nok Results: " + NokResults.toString());
            return "Gateways found: " + result.UIDs;

        }
        return "No Gateways found";
    }

    public String results(){
        return "Ok/Nok Results: " + OkResults.size() + "/" + NokResults.size() + " Handled Tags: " + handledTags.size() + " Unhandled tags: " + unhandledTags.size();
    }

    public byte[][] splitBytes(final byte[] data, final int chunkSize)
    {
        final int length = data.length;
        final byte[][] dest = new byte[(length + chunkSize - 1)/chunkSize][];
        int destIndex = 0;
        int stopIndex = 0;

        for (int startIndex = 0; startIndex + chunkSize <= length; startIndex += chunkSize)
        {
            stopIndex += chunkSize;
            dest[destIndex++] = Arrays.copyOfRange(data, startIndex, stopIndex);
        }

        if (stopIndex < length)
            dest[destIndex] = Arrays.copyOfRange(data, stopIndex, length);

        return dest;
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
        //unhandledIndices = new ArrayList<Integer>();
        unhandledTags = new ArrayList<Byte>();
        handledTags = new ArrayList<Byte>();
    }



}


