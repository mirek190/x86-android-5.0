/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package org.webrtc.videoP2P;

import java.util.List;

import org.webrtc.videoP2P.Peer.PeerType;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Filter;
import android.widget.ImageView;
import android.widget.AbsListView.LayoutParams;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.view.LayoutInflater;

public class PeerArrayAdapter extends ArrayAdapter<Peer> {
    private Context context;
    private ContactList peerList;
    private PeerType mFilter;

    public PeerArrayAdapter(Context context, ContactList objects) {
        super(context,R.layout.contacts, objects.getList());

        this.context = context;
        this.peerList = objects;
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        Peer peer = peerList.getPeerAt(position);
        if((peer==null) || (mFilter!=null && peer.getType()!=mFilter)) {
            return new LinearLayout(context);
        }

        LayoutInflater inflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View row = inflater.inflate(R.layout.contacts, parent, false);
        TextView nameView = (TextView) row.findViewById(R.id.contactrow);
        final CheckBox checkView = (CheckBox) row.findViewById(R.id.checkrow);
        ImageView typeLogo = (ImageView) row.findViewById(R.id.type_logo);


        nameView.setText(peer.getShortName());
        checkView.setTag(peer);
        checkView.setOnCheckedChangeListener(new OnCheckedChangeListener() {

                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                Peer peerObj = (Peer) checkView.getTag();
                clearAllSelection();
                peerObj.setChecked(isChecked);
                System.out.println(peerObj.getName());
                }

                });
        checkView.setChecked(peer.isChecked());
        typeLogo.setImageResource(peer.getIconResource());
        return row;
    }

    private void clearAllSelection() {
        peerList.clearSelection();
        this.notifyDataSetChanged();
    }

    void setFilter(PeerType type) {
        mFilter = type;
        this.notifyDataSetChanged();
    }
}
