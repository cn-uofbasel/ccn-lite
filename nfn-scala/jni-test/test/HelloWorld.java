class HelloWorld
{
    private native String print(String str);
    public static void main(String[] args)
    {
        System.out.println("MAIN called, number of args: " + args.length);
        // String result = new HelloWorld().print("java text");
        // System.out.println(result + " printed in Java program!");
    }

    public void javaPrint()
    {
        System.out.println("HelloWorld from Java!");
    }
    static{
        System.loadLibrary("HelloWorld");
    }
}
