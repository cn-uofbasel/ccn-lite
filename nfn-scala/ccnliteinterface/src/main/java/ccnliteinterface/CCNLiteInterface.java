//package ccnliteinterface;
//
//class CCNLiteInterface {
//
//    public native byte[] mkBinaryInterest(String str);
//    public native byte[] mkBinaryContent(String str, byte[] content);
//    public native byte[] mkAddToCacheInterest(String absoluteFilename);
//    public native String ccnbToXml(byte[] binaryInterest);
//
//    static {
//        System.loadLibrary("CCNLiteInterface");
//    }
////    public static void writeToFile(byte[] data, String filename) {
////        FileOutputStream out = null;
////        try{
////            try {
////                out = new FileOutputStream(new File(filename));
////                try {
////                    out.write(data);
////                } catch (IOException e) {
////                    e.printStackTrace();
////                }
////            } catch (FileNotFoundException e) {
////                e.printStackTrace();
////            }
////        } finally {
////            if(out != null) try {
////                out.close();
////            } catch (IOException e) {
////                e.printStackTrace();
////            }
////        }
////    }
////
////    public static void main(String[] args) {
////
////        System.out.println(new File("./").getAbsolutePath());
////
////        CCNLiteInterface ccnIf = new CCNLiteInterface();
////
////        String interestName = "javatext";
////        byte[] ccnbInterest = ccnIf.mkBinaryInterest(interestName);
////        CCNLiteInterface.writeToFile(ccnbInterest, "java.txt");
////        String xml = ccnIf.binaryInterestToXml(ccnbInterest);
////        System.out.println("XML:\n" + xml);
////
////    }
//
//
//}
//
