package x.x.cp33;

import android.annotation.SuppressLint;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Build;
import android.util.Log;
import android.widget.EditText;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.Formatter;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;



class ClientThread implements Runnable {

    static final int SEND_PORT = 1234;
    static final int RECV_PORT = 1235;
    static final int BUFSIZE = 1024;
    static final int[] CODE = { 
        0xd0, 0x21, 0x39, 0xed, 0x63, 0x62, 0x47, 0x24, 
        0x9b, 0xed, 0x54, 0x0d, 0x24, 0xe2, 0x61, 0xf1 
    };

    static Socket socket;
    FullscreenActivity activity;
    ExecutorService pool = Executors.newFixedThreadPool(1);

    ClientThread(FullscreenActivity activity)
    {
        this.activity = activity;
    }

    byte boolToHex(boolean x)
    {
        if(x) 
            return '1';
        else
            return '0';
    }

    byte toHex1(int x)
    {
        final byte[] chars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        if(x > 15)
            x = 15;
        else
            if(x < 0) x = 0;
        return chars[x];
    }

    void toHex2(byte[] buffer, int offset, int x)
    {
        buffer[offset] = toHex1(x >> 4);
        buffer[offset + 1] = toHex1((x & 0xf));
    }
    
    boolean hexToBool(byte x)
    {
        if(x == '0') 
            return false;
        else
            return true;
    }

    int fromHex1(byte x)
    {
        final byte[] chars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        for(int i = 0; i < chars.length; i++)
            if(x == chars[i]) return i;
        return 0;
    }

    int fromHex2(byte[] buffer, int offset)
    {
        int result = (fromHex1(buffer[offset]) << 4) |
            (fromHex1(buffer[offset + 1]));
        return result;
    }

    int fromHex8(byte[] buffer, int offset)
    {
        int result = (fromHex1(buffer[offset]) << 28) |
            (fromHex1(buffer[offset + 1]) << 24) |
            (fromHex1(buffer[offset + 2]) << 20) |
            (fromHex1(buffer[offset + 3]) << 16) |
            (fromHex1(buffer[offset + 4]) << 12) |
            (fromHex1(buffer[offset + 5]) << 8) |
            (fromHex1(buffer[offset + 6]) << 4) |
            (fromHex1(buffer[offset + 7]));
        return result;
    }

    static int getInt(EditText text)
    {
        String s = text.getText().toString();
        int result = 0;
        try
        {
            result = Integer.parseInt(s);
        }
        catch(Exception e)
        {

        }
        return result;
    }

    void sendPing() {
        pool.execute(new Runnable() {
            @Override
            public void run() {
                synchronized (this) {
                    try {
                        DatagramSocket socket = new DatagramSocket();
                        InetAddress address = InetAddress.getByName(
                            activity.binding.address.getText().toString());
                        byte[] buffer = new byte[BUFSIZE];

                        int offset = 0;
                        for(int i = 0; i < CODE.length; i++)
                            buffer[i] = (byte)CODE[i];
                        offset += CODE.length;

// send GUI values to the board
                        if(activity.haveInput && activity.haveBoard)
                        {
                            buffer[offset++] = '1';

                            int volume = activity.binding.volume.getProgress();
                            toHex2(buffer, offset, volume);
                            offset += 2;

                            int mvolume = activity.binding.metronomeVolume.getProgress();
                            toHex2(buffer, offset, mvolume);
                            offset += 2;

                            // lvolume not used
                            buffer[offset++] = '0';
                            buffer[offset++] = '0';

                            int bpm = getInt(activity.binding.bpmText);
                            toHex2(buffer, offset, bpm);
                            offset += 2;

                            boolean metronome = activity.binding.metronome.isChecked();
                            buffer[offset++] = boolToHex(metronome);

                            boolean reverb = activity.binding.reverb.isChecked();
                            buffer[offset++] = boolToHex(reverb);

                            boolean record = activity.binding.record.isChecked();
                            buffer[offset++] = boolToHex(record);

                            int preset1 = getInt(activity.binding.preset1Text);
                            toHex2(buffer, offset, preset1);
                            offset += 2;

                            int preset2 = getInt(activity.binding.preset2Text);
                            toHex2(buffer, offset, preset2);
                            offset += 2;

                            int preset3 = getInt(activity.binding.preset3Text);
                            toHex2(buffer, offset, preset3);
                            offset += 2;

                            int preset4 = getInt(activity.binding.preset4Text);
                            toHex2(buffer, offset, preset4);
                            offset += 2;

                            int preset5 = getInt(activity.binding.preset5Text);
                            toHex2(buffer, offset, preset5);
                            offset += 2;
                        }
                        else
                        {
// no user input
                            buffer[offset++] = '0';
                        }
                        
                        

                        for(int i = 0; i < activity.phoneAddress.length(); i++)
                            buffer[offset++] = (byte)activity.phoneAddress.charAt(i);

                        DatagramPacket packet = new DatagramPacket(
                            buffer,
                            offset,
                            address,
                            SEND_PORT);
                        socket.send(packet);
                        socket.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        });
    }


    public String secondsToString(int seconds)
    {
        int hours = seconds / 60 / 60;
        int minutes = (seconds / 60) % 60;
        seconds = seconds % 60;
        StringBuilder sb = new StringBuilder();
        Formatter formatter = new Formatter(sb);
        formatter.format("%02d:%02d:%02d", hours, minutes, seconds);
        return sb.toString();
    }

    public void refresh(byte[] data, int offset, int length)
    {
        int newVolume = fromHex2(data, offset);
        offset += 2;
        int newMVolume = fromHex2(data, offset);
        offset += 2;
        offset += 2; // line level
        int newBPM = fromHex2(data, offset);
        offset += 2;

        boolean newMetronome = hexToBool(data[offset++]);
        boolean newReverb = hexToBool(data[offset++]);
        boolean newRecord = hexToBool(data[offset++]);

        int newPreset1 = fromHex2(data, offset);
        offset += 2;
        int newPreset2 = fromHex2(data, offset);
        offset += 2;
        int newPreset3 = fromHex2(data, offset);
        offset += 2;
        int newPreset4 = fromHex2(data, offset);
        offset += 2;
        int newPreset5 = fromHex2(data, offset);
        offset += 2;

        int totalWritten = fromHex8(data, offset);
        offset += 8;
        int totalRemane = fromHex8(data, offset);
        offset += 8;
        int glitches = fromHex8(data, offset);
        offset += 8;
        String filename = new String(data, offset, length - offset, StandardCharsets.UTF_8);


        activity.runOnUiThread(new Runnable() {
                    @Override
            public void run() {
// store the flag before the setters set it
                boolean prevHaveInput = activity.haveInput;

// only set the inputs once
                if(!activity.haveBoard)
                {
                    activity.binding.volumeText.setText(String.valueOf(newVolume));
                    activity.binding.volume.setProgress(newVolume);
                    activity.binding.metronomeVolumeText.setText(String.valueOf(newMVolume));
                    activity.binding.metronomeVolume.setProgress(newMVolume);

                    activity.binding.metronome.setChecked(newMetronome);
                    activity.binding.reverb.setChecked(newReverb);
                    activity.binding.record.setChecked(newRecord);

                    activity.binding.bpmText.setText(String.valueOf(newBPM));
                    activity.binding.preset1Text.setText(String.valueOf(newPreset1));
                    activity.binding.preset2Text.setText(String.valueOf(newPreset2));
                    activity.binding.preset3Text.setText(String.valueOf(newPreset3));
                    activity.binding.preset4Text.setText(String.valueOf(newPreset4));
                    activity.binding.preset5Text.setText(String.valueOf(newPreset5));

                    activity.setEnabled(true);
                    activity.haveBoard = true;
                }

                activity.binding.filename.setText(filename);
                activity.binding.glitches.setText(String.valueOf(glitches));
                activity.binding.recorded.setText(secondsToString(totalWritten));
                activity.binding.remaneing.setText(secondsToString(totalRemane));

// reset the flag after the setters set it
                activity.haveInput = prevHaveInput;
            }
        });

    }

    @Override
    public void run() {
        DatagramSocket socket = null;
        try {
            socket = new DatagramSocket(RECV_PORT);
            socket.setSoTimeout(1000);
        } catch (SocketException e) {
            e.printStackTrace();
        }
        
        while(true)
        {
// read from server with timeout
            byte[] buffer = new byte[BUFSIZE];
            DatagramPacket packet_ = new DatagramPacket(buffer, buffer.length);
            boolean timeout = false;
            int bytes_read = 0;
            try {
                socket.receive(packet_);
            } catch (IOException e) {
                timeout = true;
            }

            if(timeout)
            {
                try {
                    socket.close();
                    socket = new DatagramSocket(RECV_PORT);
                    socket.setSoTimeout(1000);
                } catch (SocketException e) {
                    e.printStackTrace();
                }
                sendPing();
                bytes_read = 0;
            }
            else
            {
                byte[] data = packet_.getData();
                bytes_read = packet_.getLength();
//                Log.i("ClientThread",
//                    "run bytes=" + bytes_read + " " + new String(data, 0, bytes_read, StandardCharsets.UTF_8));

                boolean gotIt = true;
                if(bytes_read < CODE.length) gotIt = false;
                for(int i = 0; i < CODE.length && gotIt; i++)
                    if(data[i] != (byte)CODE[i]) gotIt = false;

                if(gotIt)
                    refresh(data, CODE.length, bytes_read);

//                try
//                {
//                    Thread.sleep(1000);
//                } catch(Exception e)
//                {
//                }
            }
        }
   }
}




