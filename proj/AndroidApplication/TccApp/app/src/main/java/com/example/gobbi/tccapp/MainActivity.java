package com.example.gobbi.tccapp;

import android.Manifest;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.BottomNavigationView;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.telephony.SmsManager;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.punchthrough.bean.sdk.Bean;
import com.punchthrough.bean.sdk.BeanDiscoveryListener;
import com.punchthrough.bean.sdk.BeanListener;
import com.punchthrough.bean.sdk.BeanManager;
import com.punchthrough.bean.sdk.message.BeanError;
import com.punchthrough.bean.sdk.message.Callback;
import com.punchthrough.bean.sdk.message.DeviceInfo;
import com.punchthrough.bean.sdk.message.ScratchBank;
import com.punchthrough.bean.sdk.message.ScratchData;

import org.w3c.dom.Text;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    EditText NameEdit;
    EditText nrEdit;
    TextView consoleTxtView;
    List<Bean> beans;
    Console mainConsole;
    Bean currentBean;

    Button button1;
    Button button2 ;
    Button button3 ;
    Button button4 ;
    byte[] scratchData;
    byte[] scratchDataOld;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final Context mainContext = this;


        consoleTxtView = (TextView) findViewById(R.id.cons);
        beans = new ArrayList<>();
        mainConsole = new Console(consoleTxtView);

        button1 = (Button) findViewById(R.id.but1);
        button2 = (Button) findViewById(R.id.but2);
        button3 = (Button) findViewById(R.id.but3);
        button4 = (Button) findViewById(R.id.but4);



        scratchData = new byte[2];
        scratchDataOld = new byte[2];
        NameEdit = (EditText)findViewById(R.id.EditTextName);
        nrEdit = (EditText)findViewById(R.id.editTextNr);




        //*******************ASK FOR PERMISSIONS***********************//


        int permissionCheck = ContextCompat.checkSelfPermission(this,
                Manifest.permission.ACCESS_COARSE_LOCATION);
        if (!(permissionCheck == PackageManager.PERMISSION_GRANTED)) {

            // Should we show an explanation?
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.ACCESS_COARSE_LOCATION)) {

            } else {
                // do request the permission
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.ACCESS_COARSE_LOCATION},
                        8);
            }
        }

        permissionCheck = ContextCompat.checkSelfPermission(this,
                Manifest.permission.ACCESS_FINE_LOCATION);
        if (!(permissionCheck == PackageManager.PERMISSION_GRANTED)) {

            // Should we show an explanation?
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.ACCESS_FINE_LOCATION)) {

            } else {
                // do request the permission
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                        8);
            }
        }

        permissionCheck = ContextCompat.checkSelfPermission(this,
                Manifest.permission.BLUETOOTH_ADMIN);
        if (!(permissionCheck == PackageManager.PERMISSION_GRANTED)) {

            // Should we show an explanation?
            if (ActivityCompat.shouldShowRequestPermissionRationale(this,
                    Manifest.permission.BLUETOOTH_ADMIN)) {

            } else {
                // do request the permission
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.BLUETOOTH_ADMIN},
                        8);
            }
        }

        //*******************SCAN BLUETOOTH***********************//
        final BeanListener beanListener = new BeanListener() {

            @Override
            public void onConnected() {
                Bean bean = beans.get(0);
                currentBean = beans.get(0);
                bean.readDeviceInfo(new Callback<DeviceInfo>() {
                    @Override
                    public void onResult(final DeviceInfo deviceInfo) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                mainConsole.addInfo("Connected to device!");
                                mainConsole.addInfo(deviceInfo.hardwareVersion());
                                mainConsole.addInfo(deviceInfo.firmwareVersion());
                                mainConsole.addInfo(deviceInfo.softwareVersion());

                            }
                        });

                    }
                });
            }

            @Override
            public void onConnectionFailed() {
                mainConsole.addErro("Failed to connect to " + beans.get(0).getDevice().getName());
            }

            @Override
            public void onDisconnected() {
                mainConsole.addWarn("Disconnected from " + beans.get(0).getDevice().getName());
            }


            @Override
            public void onSerialMessageReceived(byte[] data) {
                mainConsole.addInfo("serialMessageReceived");

            }

            @Override
            public void onScratchValueChanged(ScratchBank bank, byte[] value) {
                int temp;

                scratchData[0] = value[0];
                scratchData[1] = value[1];

                temp = value[1] & 0xFF;

                mainConsole.addInfo("ScratchValueChanged" + bank.name() + "||" + value[0] + "||" + value[1] + "->" + temp);

                int readFlame = scratchData[0] & 1<<0;
                int readShock = scratchData[0] & 1<<1;
                int st_fell = scratchData[0] & 1<<2;
                int st_emergency = (scratchData[0] & 1<<7) >> 7;
                int st_hrtbit = scratchData[0] & 1<<5;

                int readFlameOld = scratchDataOld[0] & 1<<0;
                int readShockOld = scratchDataOld[0] & 1<<1;
                int st_fellOld = scratchDataOld[0] & 1<<2;
                int st_emergencyOld = (scratchDataOld[0] & 1<<7) >> 7;
                int st_hrtbitOld = scratchDataOld[0] & 1<<5;

                Log.v("st_emergency",st_emergency + "||" + st_emergencyOld);

                if(st_emergency == 1 && st_emergencyOld == 0){
                    String name = NameEdit.getText().toString();
                    String phoneNumber = nrEdit.getText().toString();
                    String msg = name + ", preciso de ajuda! Me ligue";
                    Log.i("SMS_SENDER: AUTO IN :", msg + phoneNumber);
                    sendSMS(phoneNumber,msg);
                }else if(st_emergency == 0 && st_emergencyOld == 1){
                    String name = NameEdit.getText().toString();
                    String phoneNumber = nrEdit.getText().toString();
                    String msg = name + ", ja estou bem.";
                    Log.i("SMS_SENDER: AUTO OFF : ", msg + phoneNumber);
                    sendSMS(phoneNumber,msg);
                }else{
                    //nothing is done
                }

                scratchDataOld[0] = scratchData[0];
                scratchDataOld[1] = scratchData[1];

            }

            @Override
            public void onError(BeanError error) {
                mainConsole.addInfo("Error");
            }

            @Override
            public void onReadRemoteRssi(int rssi) {
                mainConsole.addInfo("readRemote");
            }

            // In practice you must implement the other Listener methods
        };


        final BeanDiscoveryListener listener = new BeanDiscoveryListener() {
            @Override
            public void onBeanDiscovered(Bean bean, int rssi) {
                beans.add(bean);
            }

            @Override
            public void onDiscoveryComplete() {
                currentBean = beans.get(0); //the selected bean is the first one to be found
                // This is called when the scan times out, defined by the .setScanTimeout(int seconds) method
                for (final Bean bean : beans) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mainConsole.addInfo(bean.getDevice().getName());   // "Bean"              (example)

                        }
                    });

                }

            }
        };

        BeanManager.getInstance().setScanTimeout(5);


        button1.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                BeanManager.getInstance().startDiscovery(listener);
            }
        });

        button2.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                beans.get(0).readScratchData(ScratchBank.BANK_2, new Callback<ScratchData>() {

                    @Override
                    public void onResult(final ScratchData result) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                mainConsole.addInfo("SCRATCH RESULT: " + result.getDataAsString());
                            }

                        });

                    }
                });
            }
        });

        button3.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                currentBean.connect(mainContext, beanListener);
            }
        });

        button4.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                scratchDataOld[0] = (byte)(scratchDataOld[0] | 1<<5);
                currentBean.setScratchData(ScratchBank.BANK_2,scratchDataOld);
            }
        });


    }

    private void sendSMS(String phoneNumber, String message) {
        //PendingIntent pi = PendingIntent.getActivity(this, 0,
        //        new Intent(this, Object.class), 0);
        SmsManager sms = SmsManager.getDefault();
        PendingIntent sentPI;
        String SENT = "SMS_SENT";
        sentPI = PendingIntent.getBroadcast(this, 0,new Intent(SENT), 0);
        sms.sendTextMessage(phoneNumber, null, message, sentPI, null);
    }

}    


    


