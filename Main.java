package com.jetbrains;

import javax.swing.*;
import java.awt.*;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

public class Main {

    private static void runApp() {
        MyFrame frame = new MyFrame();
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        JLabel label = new JLabel("Hello World");
        frame.getContentPane().setBackground(JdkEx.getDwmBorderColor());
        frame.getContentPane().add(label);

        Toolkit.getDefaultToolkit().addPropertyChangeListener("win.dwm.colorizationColor", (e) -> frame.getContentPane().setBackground(JdkEx.getDwmBorderColor()));
        Toolkit.getDefaultToolkit().addPropertyChangeListener("win.dwm.colorizationColorBalance", (e) -> frame.getContentPane().setBackground(JdkEx.getDwmBorderColor()));

        frame.pack();
        Dimension screen = Toolkit.getDefaultToolkit().getScreenSize();
        frame.setLocation(screen.width / 2, screen.height / 2);
        frame.setVisible(true);
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(Main::runApp);
    }
}


class MethodInvocator {
    private Method myMethod;

    public MethodInvocator(boolean warnIfAbsent, Class<?> aClass, String method, Class<?>... parameterTypes) {
        try {
            myMethod = aClass.getDeclaredMethod(method, parameterTypes);

            if (!myMethod.isAccessible()) {
                myMethod.setAccessible(true);
            }
        }
        catch (NoSuchMethodException ignored) {
        }
    }

    public boolean isAvailable() {
        return myMethod != null;
    }

    public Object invoke(Object object, Object... arguments) {
        if (!isAvailable()) {
            throw new IllegalStateException("Method is not available");
        }

        try {
            return myMethod.invoke(object, arguments);
        }
        catch (IllegalAccessException | InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }
}

class JdkEx {
    public static void setHasCustomDecoration(Window window) {
        MyCustomDecorMethods.SET_HAS_CUSTOM_DECORATION.invoke(window);
    }

    public static Color getDwmBorderColor() {
        Toolkit toolkit = Toolkit.getDefaultToolkit();
        Color colorizationColor = (Color)toolkit.getDesktopProperty("win.dwm.colorizationColor");

        Integer colorizationColorBalance = (Integer)toolkit.getDesktopProperty("win.dwm.colorizationColorBalance");
        if (colorizationColorBalance == null) // default for old JDK
            colorizationColorBalance = 89;

        if (colorizationColorBalance < 0)
            colorizationColorBalance = 100;

        Color defaultColor = new Color(0xD9D9D9);
        if (colorizationColorBalance == 0)
            return defaultColor;
        else if (colorizationColorBalance == 100)
            return colorizationColor;
        else {
            float alpha = colorizationColorBalance / 100.0f;
            float remainder = 1 - alpha;
            int r = Math.round(colorizationColor.getRed() * alpha + 0xD9 * remainder);
            int g = Math.round(colorizationColor.getGreen() * alpha + 0xD9 * remainder);
            int b = Math.round(colorizationColor.getBlue() * alpha + 0xD9 * remainder);
            return new Color(r, g, b);
        }
    }

    // lazy init
    private static class MyCustomDecorMethods {
        public static final MyMethod SET_HAS_CUSTOM_DECORATION =
                MyMethod.create(Window.class, "setHasCustomDecoration");
        public static final MyMethod SET_CUSTOM_DECORATION_HITTEST_SPOTS =
                MyMethod.create("sun.awt.windows.WWindowPeer", "setCustomDecorationHitTestSpots", List.class);
        public static final MyMethod SET_CUSTOM_DECORATION_TITLEBAR_HEIGHT =
                MyMethod.create("sun.awt.windows.WWindowPeer","setCustomDecorationTitleBarHeight", int.class);
    }

    private static class MyMethod {
        private static final MyMethod EMPTY_INSTANCE = new MyMethod(null);

        MethodInvocator myInvocator;

        public static MyMethod create( String className,  String methodName, Class<?>... parameterTypes) {
            try {
                return create(Class.forName(className), methodName, parameterTypes);
            }
            catch (ClassNotFoundException ignore) {
            }
            return EMPTY_INSTANCE;
        }

        public static MyMethod create( Class<?> cls,  String methodName, Class<?>... parameterTypes) {
            return new MyMethod(new MethodInvocator(false, cls, methodName, parameterTypes));
        }

        private MyMethod( MethodInvocator invocator) {
            this.myInvocator = invocator;
        }

        public boolean isAvailable() {
            return myInvocator != null && myInvocator.isAvailable();
        }


        public Object invoke(Object object, Object... arguments) {
            if (isAvailable()) {
                return myInvocator.invoke(object, arguments);
            }
            return null;
        }
    }

}

class MyFrame extends JFrame {
    public MyFrame() {
        JdkEx.setHasCustomDecoration(this);
    }
}
