/*
 * libjingle
 * Copyright 2013, Intel Inc.
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

import java.util.ArrayList;
import java.util.List;

import org.webrtc.videoP2P.Peer.PeerType;

import android.util.Log;

public class ContactList {
    static final String TAG = "ContactList";
    List<Peer> mList;

    public ContactList() {
        mList = new ArrayList<Peer>();
    }

    List<Peer> getList() {
        return mList;
    }

    /**
     * Return a single selection string
     */
    String getSelection() {
        String str = "";
        for(Peer p : mList)
        {
            if(p.isChecked())
            {
                str = p.getName();
            }
        }
        Log.d(TAG, "getSelection " + str);
        return str;
    }

    void clearSelection() {
        for(Peer p : mList) {
            p.setChecked(false);
        }
    }

    void add(String data) {
        boolean repetition = false;
        for(Peer p : mList)
        {
            if((p.getName()).equals(data))
            {
                repetition = true;
                break;
            }
        }
        if(!repetition) {
            Log.d(TAG, "Add Peer " + data);
            mList.add(new Peer(data));
        }
    }

    void remove(String data) {
        List<Peer> target = new ArrayList<Peer>();
        for(Peer p : mList)
        {
            if((p.getName()).equals(data))
            {
                target.add(p);
            }
        }
        for(Peer p : target) {
            Log.d(TAG, "Remove Peer " + data);
            mList.remove(p);
        }
    }

    Peer getPeerAt(int pos) {
        if(pos>=mList.size()) return null;
        return mList.get(pos);
    }

}
