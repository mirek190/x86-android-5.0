package com.intel.filemanager.util;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
    /**
     * General IO stream manipulation utilities.
     * <p>
     * This class provides static utility methods for input/output operations.
     * <p>
     * All the methods in this class that read a stream are buffered internally.
     * This means that there is no cause to use a <code>BufferedInputStream</code>
     * or <code>BufferedReader</code>. The default buffer size of 4K has been shown
     * to be efficient in tests.
     * <p>
     * Wherever possible, the methods in this class do <em>not</em> flush or close
     * the stream. This is to avoid making non-portable assumptions about the
     * streams' origin and further use. Thus the caller is still responsible for
     * closing streams after use.
     * <p>
     */
    public class IOUtils
    {
        /**
         * The default buffer size to use.
         */
    	private static final int DEFAULT_BUFFER_SIZE = 1024 * 4;
    	/**
    	 * buffer for transfer data between instream and outstream.
    	 */
    	private static byte[] buffer = new byte[DEFAULT_BUFFER_SIZE];
    	  /**
         * Copy bytes from an <code>InputStream</code> to an
         * <code>OutputStream</code>.
         * <p>
         * This method buffers the input internally, so there is no need to use a
         * <code>BufferedInputStream</code>.
         * <p>
         * Large streams (over 2GB) will return a bytes copied value of
         * <code>-1</code> after the copy has completed since the correct
         * number of bytes cannot be returned as an int. For large streams
         * use the <code>copyLarge(InputStream, OutputStream)</code> method.
         * 
         * @param input  the <code>InputStream</code> to read from
         * @param output  the <code>OutputStream</code> to write to
         * @return the number of bytes copied
         * @throws NullPointerException if the input or output is null
         * @throws IOException if an I/O error occurs
         * @throws ArithmeticException if the byte count is too large
         */
        public static int copyStream(InputStream input, OutputStream output) throws IOException {
            long count = copyLargeStream(input, output);
            if (count > Integer.MAX_VALUE) {
                return -1;
            }
            return (int) count;
        }

        /**
         * Copy bytes from a large (over 2GB) <code>InputStream</code> to an
         * <code>OutputStream</code>.
         * <p>
         * This method buffers the input internally, so there is no need to use a
         * <code>BufferedInputStream</code>.
         * 
         * @param input  the <code>InputStream</code> to read from
         * @param output  the <code>OutputStream</code> to write to
         * @return the number of bytes copied
         * @throws NullPointerException if the input or output is null
         * @throws IOException if an I/O error occurs
         */
        public static long copyLargeStream(InputStream input, OutputStream output)
                throws IOException {
            long count = 0;
            int n = 0;
            while (-1 != (n = input.read(buffer, 0, DEFAULT_BUFFER_SIZE))) {
                output.write(buffer, 0, n);
                count += n;
            }
            return count;
        }
        
        /**
         * Unconditionally close an <code>InputStream</code>.
         * <p>
         * Equivalent to {@link InputStream#close()}, except any exceptions will be ignored.
         * This is typically used in finally blocks.
         *
         * @param input  the InputStream to close, may be null or already closed
         */
        public static void closeQuietly(InputStream input) {
            try {
                if (input != null) {
                    input.close();
                }
            } catch (IOException ioe) {
                // ignore
            }
        }
        /**
         * Unconditionally close an <code>OutputStream</code>.
         * <p>
         * Equivalent to {@link OutputStream#close()}, except any exceptions will be ignored.
         * This is typically used in finally blocks.
         *
         * @param output  the OutputStream to close, may be null or already closed
         */
        public static void closeQuietly(OutputStream output) {
            try {
                if (output != null) {
                    output.close();
                }
            } catch (IOException ioe) {
                // ignore
            }
        }
    }
