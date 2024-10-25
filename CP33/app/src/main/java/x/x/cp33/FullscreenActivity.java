package x.x.cp33;

import android.annotation.SuppressLint;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.widget.CompoundButton;
import android.widget.SeekBar;

import java.net.InetAddress;
import java.util.Iterator;
import java.util.List;

import x.x.cp33.databinding.ActivityFullscreenBinding;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 */
public class FullscreenActivity extends AppCompatActivity implements SeekBar.OnSeekBarChangeListener, TextWatcher, CompoundButton.OnCheckedChangeListener, View.OnClickListener {
    ActivityFullscreenBinding binding;
    ClientThread client;
    static public String phoneAddress = "";
    // set when the GUI has read values from the board
    boolean haveBoard = false;
    // set when the user changed something
    boolean haveInput = false;

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.requestWindowFeature(Window.FEATURE_NO_TITLE);
//        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
//                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        binding = ActivityFullscreenBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        loadSettings();
        setEnabled(false);

        binding.volumeText.setInputType(InputType.TYPE_CLASS_NUMBER);
        binding.metronomeVolumeText.setInputType(InputType.TYPE_CLASS_NUMBER);
        binding.bpmText.setInputType(InputType.TYPE_CLASS_NUMBER);
        binding.preset1Text.setInputType(InputType.TYPE_CLASS_NUMBER);
        binding.preset2Text.setInputType(InputType.TYPE_CLASS_NUMBER);
        binding.preset3Text.setInputType(InputType.TYPE_CLASS_NUMBER);
        binding.preset4Text.setInputType(InputType.TYPE_CLASS_NUMBER);
        binding.preset5Text.setInputType(InputType.TYPE_CLASS_NUMBER);

        // bind all to listeners to detect user input
        binding.address.addTextChangedListener(this);
        binding.volume.setOnSeekBarChangeListener(this);
        binding.metronomeVolume.setOnSeekBarChangeListener(this);
        binding.volumeText.addTextChangedListener(this);
        binding.metronomeVolumeText.addTextChangedListener(this);
        binding.bpmText.addTextChangedListener(this);
        binding.bpmUp.setOnClickListener(this);
        binding.bpmDown.setOnClickListener(this);
        binding.apply1.setOnClickListener(this);
        binding.apply2.setOnClickListener(this);
        binding.apply3.setOnClickListener(this);
        binding.apply4.setOnClickListener(this);
        binding.apply5.setOnClickListener(this);
        binding.update1.setOnClickListener(this);
        binding.update2.setOnClickListener(this);
        binding.update3.setOnClickListener(this);
        binding.update4.setOnClickListener(this);
        binding.update5.setOnClickListener(this);
        binding.preset1Text.addTextChangedListener(this);
        binding.preset2Text.addTextChangedListener(this);
        binding.preset3Text.addTextChangedListener(this);
        binding.preset4Text.addTextChangedListener(this);
        binding.preset5Text.addTextChangedListener(this);
        binding.metronome.setOnCheckedChangeListener(this);
        binding.reverb.setOnCheckedChangeListener(this);
        binding.record.setOnCheckedChangeListener(this);



// Get the address for response packets since recvfrom just reports the router address
        ConnectivityManager cm = (ConnectivityManager)getApplicationContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        Network network = cm.getActiveNetwork();
        LinkProperties linkProperties = cm.getLinkProperties(network);
        List<LinkAddress> addresses = linkProperties.getLinkAddresses();
        Iterator<LinkAddress> i = addresses.iterator();
        while(i.hasNext())
        {
            LinkAddress l = i.next();
            InetAddress a = l.getAddress();
            Log.i("FullscreenActivity",
                    "link local=" + a.isLinkLocalAddress() +
                            " address=" + a.toString());
            if(!a.isLinkLocalAddress())
            {
                phoneAddress = a.toString().substring(1);
            }
        }


        new Thread(client = new ClientThread(this)).start();

    }

    public void loadSettings()
    {
        SharedPreferences file = getSharedPreferences("settings", 0);
        binding.address.setText(file.getString("address", "10.0.10.11"));
    }

    public void saveSettings()
    {
        Log.i("FullScreenActivity", "saveSettings " + binding.address.getText().toString());
        SharedPreferences file2 = getSharedPreferences("settings", 0);
        SharedPreferences.Editor file = file2.edit();
        file.putString("address", binding.address.getText().toString());
        file.commit();
    }

    public void setEnabled(boolean x) {
        binding.volumeText.setEnabled(x);
        binding.volume.setEnabled(x);
        binding.metronomeVolumeText.setEnabled(x);
        binding.metronomeVolume.setEnabled(x);
        binding.metronome.setEnabled(x);
        binding.reverb.setEnabled(x);
        binding.record.setEnabled(x);
        binding.bpmText.setEnabled(x);
        binding.bpmUp.setEnabled(x);
        binding.bpmDown.setEnabled(x);
        binding.apply1.setEnabled(x);
        binding.apply2.setEnabled(x);
        binding.apply3.setEnabled(x);
        binding.apply4.setEnabled(x);
        binding.apply5.setEnabled(x);
        binding.preset1Text.setEnabled(x);
        binding.preset2Text.setEnabled(x);
        binding.preset3Text.setEnabled(x);
        binding.preset4Text.setEnabled(x);
        binding.preset5Text.setEnabled(x);
        binding.update1.setEnabled(x);
        binding.update2.setEnabled(x);
        binding.update3.setEnabled(x);
        binding.update4.setEnabled(x);
        binding.update5.setEnabled(x);
        binding.filename.setEnabled(x);
        binding.glitches.setEnabled(x);
        binding.recorded.setEnabled(x);
        binding.remaneing.setEnabled(x);
    }
    public void showAll() {

    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
        haveInput = true;


        if(seekBar == binding.volume && ClientThread.getInt(binding.volumeText) != seekBar.getProgress())
            binding.volumeText.setText(String.valueOf(seekBar.getProgress()));
        if(seekBar == binding.metronomeVolume && ClientThread.getInt(binding.metronomeVolumeText) != seekBar.getProgress())
            binding.metronomeVolumeText.setText(String.valueOf(seekBar.getProgress()));
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {

    }

    @Override
    public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {

    }

    @Override
    public void afterTextChanged(Editable editable) {
        if(editable == binding.address.getEditableText())
            saveSettings();
        else
            haveInput = true;

//        Log.i("FullScreenActivity", "afterTextChanged editable=" + editable + " address=" + binding.address + " volumeText=" + binding.volumeText);

        if(editable == binding.volumeText.getEditableText() && ClientThread.getInt(binding.volumeText) != binding.volume.getProgress())
        {
            binding.volume.setProgress(ClientThread.getInt(binding.volumeText));
        }
        if(editable == binding.metronomeVolumeText.getEditableText() && ClientThread.getInt(binding.metronomeVolumeText) != binding.metronomeVolume.getProgress())
        {
            binding.metronomeVolume.setProgress(ClientThread.getInt(binding.metronomeVolumeText));
        }
    }

    @Override
    public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
        haveInput = true;
    }

    public void onClick(View view) {
        Log.i("FullScreenActivity", "onClick ");
        if(view == binding.bpmUp)
        {
            int value = ClientThread.getInt(binding.bpmText);
            value++;
            if(value > ClientThread.MAX_BPM) value = ClientThread.MAX_BPM;
            binding.bpmText.setText(Integer.toString(value));
            haveInput = true;
        }
        if(view == binding.bpmDown)
        {
            int value = ClientThread.getInt(binding.bpmText);
            value--;
            if(value < ClientThread.MIN_BPM) value = ClientThread.MIN_BPM;
            binding.bpmText.setText(Integer.toString(value));
            haveInput = true;
        }
        if(view == binding.apply1)
        {
            binding.bpmText.setText(binding.preset1Text.getText());
            haveInput = true;
        }
        if(view == binding.apply2)
        {
            binding.bpmText.setText(binding.preset2Text.getText());
            haveInput = true;
        }
        if(view == binding.apply3)
        {
            binding.bpmText.setText(binding.preset3Text.getText());
            haveInput = true;
        }
        if(view == binding.apply4)
        {
            binding.bpmText.setText(binding.preset4Text.getText());
            haveInput = true;
        }
        if(view == binding.apply5)
        {
            binding.bpmText.setText(binding.preset5Text.getText());
            haveInput = true;
        }

        if(view == binding.update1)
        {
            binding.preset1Text.setText(binding.bpmText.getText());
            haveInput = true;
        }
        if(view == binding.update2)
        {
            binding.preset2Text.setText(binding.bpmText.getText());
            haveInput = true;
        }
        if(view == binding.update3)
        {
            binding.preset3Text.setText(binding.bpmText.getText());
            haveInput = true;
        }
        if(view == binding.update4)
        {
            binding.preset4Text.setText(binding.bpmText.getText());
            haveInput = true;
        }
        if(view == binding.update5)
        {
            binding.preset5Text.setText(binding.bpmText.getText());
            haveInput = true;
        }
    }

    @Override
    public void onPointerCaptureChanged(boolean hasCapture) {

    }
}
