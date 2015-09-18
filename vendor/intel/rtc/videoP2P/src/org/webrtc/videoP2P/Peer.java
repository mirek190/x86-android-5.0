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

public class Peer {
    private String name;
    private String status;
    private boolean isChecked;
    private PeerType type;

    enum PeerType {
        INTEL,
        GMAIL,
        UNKNOWN
    }

    public Peer(String name) {
        this.name = name;
        this.type = parsePeerType(name);
        isChecked = false;
    }

    public void setChecked(boolean checked) {
        isChecked = checked;
    }

    public boolean isChecked() {
        return isChecked;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

    public String getShortName() {
        int offset = name.lastIndexOf("/");
        return new String(name.substring(0, offset));
    }

    public PeerType getType() {
        return type;
    }

    static PeerType parsePeerType(String data) {
        int offset = data.indexOf("/") + 1;
        if(data.startsWith("rtcdemo", offset)) return PeerType.INTEL;
        else if(data.startsWith("gmail", offset)) return PeerType.GMAIL;
        else return PeerType.UNKNOWN;
    }

    int getIconResource() {
        if(type==PeerType.INTEL) return R.drawable.intel;
        else if(type==PeerType.GMAIL) return R.drawable.gmail;

        return 0;
    }
}
