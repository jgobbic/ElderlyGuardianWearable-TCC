package com.example.gobbi.tccapp;

import android.widget.TextView;

/**
 * Created by gobbi on 9/29/2017.
 */

public class Console {
    private String consoleTxt;
    private TextView consoleTxtView;

    Console(TextView thisConsoleTextView){
        consoleTxtView = thisConsoleTextView;
        consoleTxt =  consoleTxt + "Console:\n";
        consoleTxtView.setText(consoleTxt);
    }
    public String addWarn(String warnmsg){
        consoleTxt = consoleTxt + "W:" + warnmsg + "\n";
        consoleTxtView.setText(consoleTxt);
        return consoleTxt;
    }
    public String addErro(String erromsg){

        consoleTxt = consoleTxt + "E:" + erromsg + "\n";
        consoleTxtView.setText(consoleTxt);
        return consoleTxt;
    }
    public String addInfo(String infomsg){

        consoleTxt = consoleTxt + "I:" + infomsg + "\n";
        consoleTxtView.setText(consoleTxt);
        return consoleTxt;
    }


}
