//

package ch.unibas.ccn_lite_android;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import android.app.Activity;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.Window;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;


public class CcnLiteAndroid extends Activity
{
    ArrayAdapter adapter;
    TextView debuglevel;
    BluetoothAdapter BTadapter;
    BluetoothGatt bleConnection;
    BluetoothGattService bleService;
    byte bleAddr[] = new byte[6];
    String hello;
    Context ccnLiteContext;
    int newData;

    public final static UUID SERV_UUID = new UUID(0x0000222000001000L,
                                                  0x800000805f9b34fbL);
    public final static UUID CONF_UUID = new UUID(0x0000290200001000L,
                                                  0x800000805f9b34fbL);
    public final static UUID SEND_UUID = new UUID(0x0000222200001000L,
                                                  0x800000805f9b34fbL);
    public final static UUID RECV_UUID = new UUID(0x0000222100001000L,
                                                  0x800000805f9b34fbL);

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main_layout);

        adapter = new ArrayAdapter(this, R.layout.logtextview, 0);

        if (!getPackageManager().hasSystemFeature(
                                      PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, R.string.ble_not_supported,
                           Toast.LENGTH_SHORT).show();
            adapter.add("sys: BLE not supported");
        } else { // Initializes Bluetooth adapter
            final BluetoothManager bluetoothManager =
                (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            BTadapter = bluetoothManager.getAdapter();

            adapter.add("sys: BLE is supported");
            if (BTadapter == null || !BTadapter.isEnabled()) {
                Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                adapter.add("sys: BLE not yet enabled");
                //                startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
                startActivityForResult(enableBtIntent, 1);
            }
        }
        adapter.notifyDataSetChanged();

        hello = relayInit();
        ccnLiteContext = this;
    }

    @Override
    public void onStart()
    {
        ListView lv;

        super.onStart();

        lv = (ListView) findViewById(R.id.listview);
        lv.setAdapter(adapter);

        TextView tv = (TextView) findViewById(R.id.loglink);
        tv.setText(Html.fromHtml("Status console &gt;&gt; " +
                                 "<a href=\"http://localhost:8080/\">" +
                                 hello + "  </a> "));
        tv.setMovementMethod(LinkMovementMethod.getInstance());

        debuglevel = (TextView) findViewById(R.id.debuglevel);

        tv = (TextView) findViewById(R.id.transport);
        tv.setText("Transport: " + relayGetTransport());

        tv = (TextView) findViewById(R.id.debuglevel);
        tv.setText(Html.fromHtml("loglevel &le; debug"));

        Button b = (Button) findViewById(R.id.plus);
        b.setText(">>");
        b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    debuglevel.setText(Html.fromHtml(relayPlus()));
                }
            });
        b = (Button) findViewById(R.id.minus);
        b.setText("<<");
        b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    debuglevel.setText(Html.fromHtml(relayMinus()));
                }
            });
        b = (Button) findViewById(R.id.dump);
        b.setOnClickListener(new View.OnClickListener() {
                public void onClick(View v) {
                    relayDump();
                }
            });

        mHandler = new Handler();
        scanLeDevice(true);
    }

    public void appendToLog(String line) {
        while (adapter.getCount() > 500)
            adapter.remove(adapter.getItem(0));
        adapter.add(line);
        adapter.notifyDataSetChanged();
    }

    public native String relayInit();
    public native String relayGetTransport();

    public native String relayPlus();
    public native String relayMinus();
    public native void relayDump();
    public native void relayTimer();
    public native void relayRX(byte[] addr, byte[] data);

    /* this is used to load the 'ccn-lite-android' library on application
     * startup. The library has already been unpacked into
     * /data/data/ch.unibas.ccnliteandroid/lib/libccn-lite-android.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("ccn-lite-android");
    }

    // ----------------------------------------------------------------------


    private boolean mScanning;
    private Handler mHandler;

    // Stops scanning after 10 seconds.
    private static final long SCAN_PERIOD = 10000;

    private void scanLeDevice(final boolean enable) {
        if (enable) {
            // Stops scanning after a pre-defined scan period.
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    mScanning = false;
                    BTadapter.stopLeScan(mLeScanCallback);
                }
                }, SCAN_PERIOD);

            mScanning = true;
            BTadapter.startLeScan(mLeScanCallback);
        } else {
            mScanning = false;
            BTadapter.stopLeScan(mLeScanCallback);
        }
    }

    //private LeDeviceListAdapter mLeDeviceListAdapter;

    // Device scan callback.
    private BluetoothAdapter.LeScanCallback mLeScanCallback =
        new BluetoothAdapter.LeScanCallback() {
            public void debugMsg(String msg) {
                final String str = msg;
                runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            appendToLog(str);
                        }
                    });
            }
            @Override
            public void onLeScan(final BluetoothDevice device, int rssi,
                                 byte[] scanRecord) {
                runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            scanLeDevice(false);
                            // adapter.addDevice(device);
                            debugMsg("addr=" + device.getAddress());
                            String[] baddr = device.getAddress().split(":");
                            for (int i = 0; i < 6; i++) {
                                bleAddr[i] = (byte) Integer.parseInt(baddr[i], 16);
                            }
                            adapter.notifyDataSetChanged();
                            bleConnection = device.connectGatt(ccnLiteContext,
                                                               false,
                                                               bleConnectCB);
                            appendToLog("after connect");

                        }
                    });
            }
        };


    private final BluetoothGattCallback bleConnectCB =
        new BluetoothGattCallback() {
            public void debugMsg(String msg) {
                final String str = msg;
                runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            appendToLog(str);
                        }
                    });
            }
            @Override
            public void onConnectionStateChange(BluetoothGatt gatt,
                                                int status, int newState) {
                String intentAction;

                debugMsg("  onConnectionStateChange " + gatt);
                debugMsg("  onConnectionStateChange " + status +
                            " " + newState);
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    debugMsg("BLE connected!");

                    BluetoothGattCharacteristic c1 =
                        new BluetoothGattCharacteristic(RECV_UUID,
                             BluetoothGattCharacteristic.PROPERTY_READ,
                             BluetoothGattCharacteristic.PERMISSION_READ);
                    debugMsg("  read=" +
                                bleConnection.readCharacteristic(c1));

                    //                    debugMsg("BLE MTU is " + bleConnection.requestMtu(50));

                    bleConnection.discoverServices();
                    /*
                      intentAction = ACTION_GATT_CONNECTED;
                      mConnectionState = STATE_CONNECTED;
                      broadcastUpdate(intentAction);
                      Log.i(TAG, "Connected to GATT server.");
                      Log.i(TAG, "Attempting to start service discovery:" +
                      mBluetoothGatt.discoverServices());
                    */
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    debugMsg("BLE disconnected\n");
                    /*
                      intentAction = ACTION_GATT_DISCONNECTED;
                      mConnectionState = STATE_DISCONNECTED;
                      Log.i(TAG, "Disconnected from GATT server.");
                      broadcastUpdate(intentAction);
                    */
                }
            }
            @Override
            public void onServicesDiscovered(BluetoothGatt gatt, int status) {
                debugMsg("services discovered! " + status);

                if (status == BluetoothGatt.GATT_SUCCESS) {
                    debugMsg("  success");

                    bleService = gatt.getService(SERV_UUID);
                    if (bleService == null) {
                        debugMsg("no service!");
                        return;
                    }
                    BluetoothGattCharacteristic recvC =
                        bleService.getCharacteristic(RECV_UUID);
                    if (recvC == null)  {
                        debugMsg("no RECV characteristic!");
                        return;
                    }
                    BluetoothGattDescriptor recvD =
                        recvC.getDescriptor(CONF_UUID);
                    if (recvD == null) {
                        debugMsg("no RECV descriptor!");
                        return;
                    }
                    debugMsg("setChar returns " +
                           gatt.setCharacteristicNotification(recvC, true));
                    debugMsg("setVal returns " + recvD.setValue(
                        BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE));
                    debugMsg("writeDescr returns " +
                             gatt.writeDescriptor(recvD));

                    if ((recvC.getProperties() & BluetoothGattCharacteristic.PROPERTY_READ) == 0) {
                        debugMsg(" no read property");
                    }

                    /*
                    debugMsg("readChar returns " +
                             gatt.readCharacteristic(recvC));
                    */


                    BluetoothGattCharacteristic sendC =
                        bleService.getCharacteristic(SEND_UUID);
                    if (sendC == null)  {
                        debugMsg("no SEND characteristic!");
                        return;
                    }
                    if ((sendC.getProperties() & BluetoothGattCharacteristic.PROPERTY_WRITE) == 0) {
                        debugMsg(" no send property");
                    }

                    /*
                    sendC.setValue("holla");
                    sendC.setWriteType(
                       BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
                    debugMsg("  send returned " +
                             gatt.writeCharacteristic(sendC));
                    */

                    debugMsg("  success, done");
                }

                /*
                List<BluetoothGattService> services =
                    bleConnection.getServices();
                for (BluetoothGattService service : services) {
                    debugMsg("  service " + service.getUuid());

                    List<BluetoothGattCharacteristic> characteristics =
                        service.getCharacteristics();
                    for (BluetoothGattCharacteristic ch : characteristics) {
                        debugMsg("    characteristic " + ch.getUuid());
                    }
                }
                */

                debugMsg("SERV " + SERV_UUID);
                debugMsg("CONF " + CONF_UUID);
                debugMsg("SEND " + SEND_UUID);
                debugMsg("RECV " + RECV_UUID);
            }

            @Override
            public void onCharacteristicRead(BluetoothGatt gatt,
                                 BluetoothGattCharacteristic characteristic,
                                 int status) {
                if (newData > 0) {
                    debugMsg("onChRead0");
                    if (status == BluetoothGatt.GATT_SUCCESS) {
                        debugMsg("  readChar returns " +
                             gatt.readCharacteristic(characteristic) +
                             " cnt=" +
                             characteristic.getValue().length);
//                  broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
//                        newData -= 1;
                    }
                }
            }

            @Override
            public void onCharacteristicChanged(BluetoothGatt gatt,
                                 BluetoothGattCharacteristic characteristic) {
                if (!gatt.readCharacteristic(characteristic)) {
                    debugMsg("onChChange - readChar returns false");
                } else {
                    final byte[] val = characteristic.getValue();
                    runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                relayRX(bleAddr, val);
                            }
                        });
                }
            }

        };

    public void bleSend(byte[] data)
    {
        if (bleConnection == null || bleService == null) {
            appendToLog("bleSend: BLE not initialized");
            return; // false;
        }

        BluetoothGattCharacteristic sendC =
            bleService.getCharacteristic(SEND_UUID);
        if (sendC == null) {
            appendToLog("bleSend: no send characteristic");
            return; // false;
        }

        sendC.setValue(data);
        sendC.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
        boolean b = bleConnection.writeCharacteristic(sendC);
        if (!b)
            appendToLog("bleSend: write characteristics failed");
        return;

    }
}
