package com.nordicsemi.nrfUARTv2;

import org.json.JSONArray;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.io.Writer;
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

    public byte[] GenerateALP(String accessProfile, String fileID, String fileOffset, String fileLength){

        byte accessProfileByte[] = hexStringToByteArray(accessProfile);
        byte fileIDByte[] = hexStringToByteArray(fileID);
        byte fileOffsetByte[] = hexStringToByteArray(fileOffset);
        byte fileLengthByte[] = hexStringToByteArray(fileLength);

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

        /*byte[] ALP = {  (byte)0xb4, //tag response action
                TAG[0], //tag
                (byte)0x32,
                (byte)0xd7,
                (byte)0x01,
                (byte)0x00,
                (byte)0x10,
                (byte)0x01, //access class
                (byte)0x01, //action (read)
                (byte)0x00, //firmware file (uid)
                (byte)0x00, //offset
                (byte)0x08};//length*/

        byte[] ALP = {  (byte)0xb4, //tag response action
                        TAG[0], //tag
                        (byte)0x32,
                        (byte)0xd7,
                        (byte)0x01,
                        (byte)0x00,
                        (byte)0x10,
                        accessProfileByte[0], //access class
                        (byte)0x01, //action (read)
                        fileIDByte[0], //firmware file (uid)
                        fileOffsetByte[0], //offset
                        fileLengthByte[0]};//length

        command = concat(command, ALP);

        //set serial length byte to length of ALP command
        command[6] = (byte)ALP.length;

        System.out.println("ALP: " + MainActivity.byteArrayToHexString(command));

        return command;
    }

    public String ParseALP(byte[] response){
        System.out.println("Response " + response);
        if(response[response.length - 2] == (byte)0xe3 && response[response.length - 3] == (byte)0x02 && response[response.length - 4] == (byte)0x00 && response[response.length - 5] == (byte)0xc0)
        {
            byte tag = response[response.length-1];
            unhandledTags.remove(unhandledTags.indexOf(tag));
            handledTags.add(tag);
            NokResults.add(new Result(MainActivity.currentLocation, tag));
            System.out.println("Ok Results: " + OkResults.toString());
            System.out.println("Nok Results: " + NokResults.toString());

            return "No response";
        }
        else if(response[response.length - 2] == (byte)0xa3 && response[response.length - 3] == (byte)0x02 && response[response.length - 4] == (byte)0x00)
        {
            //cut off last short response, only keep UID responses
            byte ALPCommands[] = Arrays.copyOfRange(response, 0, response.length-5);

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

            if(MainActivity.defaultParameters)
            {
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
            else
            {
                //cut array into chunks of 39 (chunk per gateway)
                //TODO: fix parsing other commands
                return "Response: " + MainActivity.byteArrayToHexString(ALPCommands);
            }


        }
        else
            return "NOK";
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

    public void saveJSON(){
        JSONArray jsonArray = new JSONArray();
        for (int i=0; i < OkResults.size(); i++) {
            jsonArray.put(OkResults.get(i).getJSONObject());
        }
        for (int i=0; i < NokResults.size(); i++) {
            jsonArray.put(NokResults.get(i).getJSONObject());
        }
        System.out.println(jsonArray);

        Writer output = null;
        String currentDateString = DateFormat.getDateInstance().format(new Date());
        String currentTimeString = DateFormat.getTimeInstance().format(new Date());
        String fileName = "Survey Result " + currentDateString + "-" + currentTimeString + ".json";
        String baseDir = android.os.Environment.getExternalStorageDirectory().getAbsolutePath();
        String filePath = baseDir + File.separator + fileName;

        File file = new File(filePath);
        try {
            output = new BufferedWriter(new FileWriter(file));
            output.write(jsonArray.toString());
            output.close();
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

    public static byte[] hexStringToByteArray(String s) {
        int len = s.length();
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i+1), 16));
        }
        return data;
    }



}


