package org.bouncycastle.crypto.paddings;

import org.bouncycastle.crypto.BlockCipher;
import org.bouncycastle.crypto.BufferedBlockCipher;
import org.bouncycastle.crypto.CipherParameters;
import org.bouncycastle.crypto.DataLengthException;
import org.bouncycastle.crypto.InvalidCipherTextException;
import org.bouncycastle.crypto.OutputLengthException;
import org.bouncycastle.crypto.params.ParametersWithRandom;
import org.bouncycastle.crypto.params.KeyParameter;
import org.bouncycastle.crypto.params.ParametersWithIV;
import org.bouncycastle.crypto.modes.CBCBlockCipher;

/**
 * A wrapper class that allows block ciphers to be used to process data in
 * a piecemeal fashion with padding. The PaddedBufferedBlockCipher
 * outputs a block only when the buffer is full and more data is being added,
 * or on a doFinal (unless the current block in the buffer is a pad block).
 * The default padding mechanism used is the one outlined in PKCS5/PKCS7.
 */
public class PaddedBufferedBlockCipher
    extends BufferedBlockCipher
{
    // load the aesni library.
    private static boolean sLibaesniLoaded = true;
    private boolean mUseAesniLib = false;
    static
    {
        try
        {
            System.loadLibrary("aesni");
        } catch(UnsatisfiedLinkError e) {
            sLibaesniLoaded = false;
        }
    };
    // Local key,IV and native method declarations
    private byte[] aesKey;
    private byte[] iV;
    // The AESNI Encrypt,Decrypt function
    private native int aesNI(byte[] in, byte[] key, byte[] out, byte[] iv, int len,
                             int enc, int keylen);
    // Check if hardware supports AESNI
    private native int checkAesNI();
    BlockCipherPadding  padding;

    /**
     * Create a buffered block cipher with the desired padding.
     *
     * @param cipher the underlying block cipher this buffering object wraps.
     * @param padding the padding type.
     */
    public PaddedBufferedBlockCipher(
        BlockCipher         cipher,
        BlockCipherPadding  padding)
    {
        this.cipher = cipher;
        this.padding = padding;
        this.iV = new byte[cipher.getBlockSize()];

        buf = new byte[cipher.getBlockSize()];
        bufOff = 0;
    }

    /**
     * Create a buffered block cipher PKCS7 padding
     *
     * @param cipher the underlying block cipher this buffering object wraps.
     */
    public PaddedBufferedBlockCipher(
        BlockCipher     cipher)
    {
        this(cipher, new PKCS7Padding());
    }

    /**
     * initialise the cipher.
     *
     * @param forEncryption if true the cipher is initialised for
     *  encryption, if false for decryption.
     * @param params the key and other data required by the cipher.
     * @exception IllegalArgumentException if the params argument is
     * inappropriate.
     */
    public void init(
        boolean             forEncryption,
        CipherParameters    params)
        throws IllegalArgumentException
    {
        this.forEncryption = forEncryption;
        mUseAesniLib = (sLibaesniLoaded == true &&
                               checkAesNI() == 1 &&
                               cipher.getAlgorithmName().equals("AES/CBC"));

        reset();

        if (params instanceof ParametersWithRandom)
        {
            ParametersWithRandom    p = (ParametersWithRandom)params;

            padding.init(p.getRandom());
            if (mUseAesniLib == true)
            {
                if (p.getParameters() instanceof ParametersWithIV)
                {
                    ParametersWithIV ivParam = (ParametersWithIV)p.getParameters();
                    byte[] iv = ivParam.getIV();
                    this.aesKey = ((KeyParameter)ivParam.getParameters()).getKey();
                    System.arraycopy(iv, 0, iV, 0, iv.length); // is array copy really needed (align iv)?
                }
                else
                {
                    this.aesKey = ((KeyParameter)p.getParameters()).getKey();
                }
            }
            cipher.init(forEncryption, p.getParameters());
        }
        else
        {
            padding.init(null);
            if (mUseAesniLib == true)
            {
                if (params instanceof ParametersWithIV)
                {
                    ParametersWithIV ivParam = (ParametersWithIV)params;
                    byte[] iv = ivParam.getIV();
                    this.aesKey = ((KeyParameter)ivParam.getParameters()).getKey();
                    System.arraycopy(iv, 0, iV, 0, iv.length); // is array copy really needed (align iv)?
                }
                else
                {
                    this.aesKey = ((KeyParameter)params).getKey();
                }
            }
            cipher.init(forEncryption, params);
        }
    }

    /**
     * return the minimum size of the output buffer required for an update
     * plus a doFinal with an input of len bytes.
     *
     * @param len the length of the input.
     * @return the space required to accommodate a call to update and doFinal
     * with len bytes of input.
     */
    public int getOutputSize(
        int len)
    {
        int total       = len + bufOff;
        int leftOver    = total % buf.length;

        if (leftOver == 0)
        {
            if (forEncryption)
            {
                return total + buf.length;
            }

            return total;
        }

        return total - leftOver + buf.length;
    }

    /**
     * return the size of the output buffer required for an update
     * an input of len bytes.
     *
     * @param len the length of the input.
     * @return the space required to accommodate a call to update
     * with len bytes of input.
     */
    public int getUpdateOutputSize(
        int len)
    {
        int total       = len + bufOff;
        int leftOver    = total % buf.length;

        if (leftOver == 0)
        {
            return total - buf.length;
        }

        return total - leftOver;
    }

    /**
     * process a single byte, producing an output block if neccessary.
     *
     * @param in the input byte.
     * @param out the space for any output that might be produced.
     * @param outOff the offset from which the output will be copied.
     * @return the number of output bytes copied to out.
     * @exception DataLengthException if there isn't enough space in out.
     * @exception IllegalStateException if the cipher isn't initialised.
     */
    public int processByte(
        byte        in,
        byte[]      out,
        int         outOff)
        throws DataLengthException, IllegalStateException
    {
        int         resultLen = 0;

        if (bufOff == buf.length)
        {
            resultLen = cipher.processBlock(buf, 0, out, outOff);
            bufOff = 0;
        }

        buf[bufOff++] = in;

        return resultLen;
    }

    /**
     * process an array of bytes, producing output if necessary.
     *
     * @param in the input byte array.
     * @param inOff the offset at which the input data starts.
     * @param len the number of bytes to be copied out of the input array.
     * @param out the space for any output that might be produced.
     * @param outOff the offset from which the output will be copied.
     * @return the number of output bytes copied to out.
     * @exception DataLengthException if there isn't enough space in out.
     * @exception IllegalStateException if the cipher isn't initialised.
     */
    public int processBytes(
        byte[]      in,
        int         inOff,
        int         len,
        byte[]      out,
        int         outOff)
        throws DataLengthException, IllegalStateException
    {
        if (len < 0)
        {
            throw new IllegalArgumentException("Can't have a negative input length!");
        }

        int blockSize   = getBlockSize();
        int length      = getUpdateOutputSize(len);

        if (length > 0)
        {
            if ((outOff + length) > out.length)
            {
                throw new OutputLengthException("output buffer too short");
            }
        }

        int resultLen = 0;
        int gapLen = buf.length - bufOff;

        if (len > gapLen)
        {
            System.arraycopy(in, inOff, buf, bufOff, gapLen);

            resultLen += cipher.processBlock(buf, 0, out, outOff);

            bufOff = 0;
            len -= gapLen;
            inOff += gapLen;

            if (mUseAesniLib == true)
            {
                // check for valid key length
                if (aesKey.length != 32 && aesKey.length != 16 && aesKey.length != 24)
                {
                    throw new IllegalArgumentException("Key size should be 128,192,256 bits");
                }

                // AESNI found, use this instead
                int rem = 0;
                rem = len - 1;
                rem -= rem%16;
                int retsize = 0;
                // If the len of remaining input is greater than the block size, process it in native
                if (len > buf.length)
                {
                    retsize = processNative(in, inOff, out, outOff + resultLen, rem);
                }
                len -= retsize;
                resultLen += retsize;
                inOff += retsize;
            }
            else
            {
                // Call the local process native function instead of the while loop calling into AESFastEngine
                while (len > buf.length)
                {
                    resultLen += cipher.processBlock(in, inOff, out, outOff + resultLen);
                    len -= blockSize;
                    inOff += blockSize;
                }

            }
        }

        System.arraycopy(in, inOff, buf, bufOff, len);

        bufOff += len;

        return resultLen;
    }

    /**
     * process an array of bytes in native methods, producing output if necessary.
     * @brief processNative-This functions processes blocks of data in native code
     * @param in the input byte array.
     * @param inOff the offset at which the input data starts.
     * @param inputLen the number of bytes to be copied out of the input array.
     * @param out the space for any output that might be produced.
     * @param outOff the offset from which the output will be copied.
     * @return the number of output bytes copied to out.
     * @exception DataLengthException if there isn't enough space in out.
     * @exception IllegalStateException if the cipher isn't initialised.
     */
    private int processNative(
        byte[]      in,
        int         inOff,
        byte[]      out,
        int         outOff,
        int         inputLen)
        throws DataLengthException, IllegalStateException
    {
        int blockSize = cipher.getBlockSize();
        int len = 0;
        if (cipher instanceof CBCBlockCipher)
        {
            CBCBlockCipher cbc = (CBCBlockCipher)cipher;
            byte[] input = new byte[inputLen];
            byte[] output = new byte[inputLen];
            System.arraycopy(in, inOff, input, 0, inputLen);       // TODO: arraycopy is it necessary
            byte[] cbV = new byte[blockSize];
            System.arraycopy(cbc.getCbcV(), 0, cbV, 0, blockSize); // TODO: arraycopy is it necessary
            /* Pass '1' to the native if the current operation is encryption else
             * pass a '0' indicating decryption
             */
            if (forEncryption == true)
            {
                len = aesNI(input, aesKey, output, cbV, inputLen, 1, aesKey.length);
            }
            else
            {
                len = aesNI(input, aesKey, output, cbV, inputLen, 0, aesKey.length);
            }
            System.arraycopy(output, 0, out, outOff, len);
            if (forEncryption)
            {
                System.arraycopy(output, len - 16, cbV, 0, blockSize);
                cbc.setCbcV(cbV);
            }
            else
            {
                System.arraycopy(input, inputLen - 16, cbV, 0, blockSize);
                cbc.setCbcV(cbV);
            }
        }
        return len;

    }

    /**
     * Process the last block in the buffer. If the buffer is currently
     * full and padding needs to be added a call to doFinal will produce
     * 2 * getBlockSize() bytes.
     *
     * @param out the array the block currently being held is copied into.
     * @param outOff the offset at which the copying starts.
     * @return the number of output bytes copied to out.
     * @exception DataLengthException if there is insufficient space in out for
     * the output or we are decrypting and the input is not block size aligned.
     * @exception IllegalStateException if the underlying cipher is not
     * initialised.
     * @exception InvalidCipherTextException if padding is expected and not found.
     */
    public int doFinal(
        byte[]  out,
        int     outOff)
        throws DataLengthException, IllegalStateException, InvalidCipherTextException
    {
        int blockSize = cipher.getBlockSize();
        int resultLen = 0;

        if (forEncryption)
        {
            if (bufOff == blockSize)
            {
                if ((outOff + 2 * blockSize) > out.length)
                {
                    reset();

                    throw new OutputLengthException("output buffer too short");
                }

                resultLen = cipher.processBlock(buf, 0, out, outOff);
                bufOff = 0;
            }

            padding.addPadding(buf, bufOff);

            resultLen += cipher.processBlock(buf, 0, out, outOff + resultLen);

            reset();
        }
        else
        {
            if (bufOff == blockSize)
            {
                resultLen = cipher.processBlock(buf, 0, buf, 0);
                bufOff = 0;
            }
            else
            {
                reset();

                throw new DataLengthException("last block incomplete in decryption");
            }

            try
            {
                resultLen -= padding.padCount(buf);

                System.arraycopy(buf, 0, out, outOff, resultLen);
            }
            finally
            {
                reset();
            }
        }

        return resultLen;
    }
}
