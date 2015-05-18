// 

package ch.unibas.ccn_lite_android;

import java.util.ArrayList;

import android.app.Activity;
import android.widget.ArrayAdapter;
import android.app.Activity;
import android.view.Window;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.widget.ListView;
import android.widget.TextView;

import android.graphics.drawable.Drawable;
import android.os.Bundle;


public class CcnLiteAndroid extends Activity
{
    ListView lv;
    ArrayAdapter adapter;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main_layout);
 
        /*
        Drawable d = getResources().getDrawable(
                                           R.drawable.ccn_lite_logo_712x184);
        */

        //        String[] list = {};
        lv = (ListView) findViewById(R.id.listview);
        adapter = new ArrayAdapter(this, R.layout.logtextview, 0);
        lv.setAdapter(adapter);

        TextView tv = (TextView) findViewById(R.id.loglink);
        tv.setText(Html.fromHtml("Status console &gt;&gt; <a href=\"http://localhost:8080/\">" +
                                 relayInit() + "  </a>"));
        tv.setMovementMethod(LinkMovementMethod.getInstance());

        tv = (TextView) findViewById(R.id.hello);
        tv.setText("Transport: " + relayGetTransport());
    }

    public void appendToLog(String line) {
        while (adapter.getCount() > 500)
            adapter.remove(0);
        adapter.add(line);
        adapter.notifyDataSetChanged();
    }

    public native String relayInit();
    public native String relayGetTransport();

    /* this is used to load the 'ccn-lite-android' library on application
     * startup. The library has already been unpacked into
     * /data/data/ch.unibas.ccnliteandroid/lib/libccn-lite-android.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("ccn-lite-android");
    }
}

