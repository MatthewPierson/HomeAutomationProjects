package com.matty.garagedoor;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    public void clickMe(View view)
    {
        UDP_Client Client = new UDP_Client();
        Client.Message = "messageGoesHere"; // Message to send to server
        Client.port = 1234; // Port of server
        Client.IPAddress = "192.168.1.1"; // IP Address of server
        Client.sendMessage();
        Toast.makeText(getApplicationContext(),"UDP packet sent with message: \"" + Client.Message + "\"",Toast.LENGTH_LONG).show();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }
}