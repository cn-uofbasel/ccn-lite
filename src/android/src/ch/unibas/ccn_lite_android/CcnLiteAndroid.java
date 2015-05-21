// 

package ch.unibas.ccn_lite_android;

import java.util.ArrayList;

import android.app.Activity;
import android.widget.ArrayAdapter;
import android.app.Activity;
import android.view.View;
import android.view.Window;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

import android.graphics.drawable.Drawable;
import android.os.Bundle;


public class CcnLiteAndroid extends Activity
{
    ArrayAdapter adapter;
    TextView debuglevel;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main_layout);

        adapter = new ArrayAdapter(this, R.layout.logtextview, 0);

        /*
        Thread t = new Thread(){
                public void run() {
                    while(true) {
                        relayTimer();
                        try {
                            Thread.sleep(60 * 1000);
                        } catch (Exception e) {
                        }
                    }
                }
            };
        t.start();
        */
    }

    @Override
    public void onStart()
    {
        ListView lv;

        super.onStart();

        debuglevel = (TextView) findViewById(R.id.debuglevel);
 
        lv = (ListView) findViewById(R.id.listview);
        lv.setAdapter(adapter);

        TextView tv = (TextView) findViewById(R.id.loglink);
        tv.setText(Html.fromHtml("Status console &gt;&gt; <a href=\"http://localhost:8080/\">" +
                                 relayInit() + "  </a> "));
        tv.setMovementMethod(LinkMovementMethod.getInstance());

        tv = (TextView) findViewById(R.id.hello);
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

    /* this is used to load the 'ccn-lite-android' library on application
     * startup. The library has already been unpacked into
     * /data/data/ch.unibas.ccnliteandroid/lib/libccn-lite-android.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("ccn-lite-android");
    }
}

