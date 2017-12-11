package com.nordicsemi.nrfUARTv2;

import java.util.ArrayList;
import java.util.Arrays;
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
    //idee, array met gestuurde tags, opslagen met locatie. Ene met succesvolle, ene met dead spots

    public byte[] GenerateALP(){

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
        NokResults.add(new Result(MainActivity.currentLocation, TAG[0]));
        unhandledIndices.add(NokResults.size()-1);

        byte[] ALP = {  (byte)0xb4, //tag response action
                        TAG[0], //tag
                        (byte)0x01, //action (ReadFileData)
                        (byte)0x00, //file-id
                        (byte)0x00, //offset
                        (byte)0x01}; //length
        command = concat(command, ALP);

        //set serial length byte to length of ALP command
        command[6] = (byte)ALP.length;

        return command;
    }

    public String ParseALP(byte[] response){
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



}


