/*
 * Copyright (c) 2026, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
package test.javafx.scene.control;

import com.sun.javafx.PlatformUtil;
import java.util.concurrent.CountDownLatch;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Spinner;
import javafx.scene.control.TextField;
import javafx.scene.control.TextFormatter;
import javafx.scene.layout.GridPane;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.stage.Stage;
import javafx.stage.Window;
import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import test.util.Util;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

// Verify that an ENTER key press in a TextField is not consumed. This ensures
// the ENTER can make it all the way to the Scene and trigger the default OK
// button in a dialog. This should work whether the TextField is stand-alone
// or an editor in a ComboBox or Spinner. It should also work whether the
// TextField has a formatter associated with it or not (when a formatter is
// involved ENTER also commits the value of the text).
//
// These tests also verify that ESCAPE can trigger the default Cancel button
// in a dialog. When a formatter is attached ESCAPE should first try to
// cancel editing. If the text doesn't change the ESCAPE should be allowed to
// bubble up to the Scene; otherwise it should be consumed.
public class TextFieldHandlesEnterEscape {
    private static CountDownLatch startupLatch = new CountDownLatch(1);

    private static volatile Stage stage;

    private static volatile TextField textField;
    private static volatile TextField formattedTextField;
    private static volatile ComboBox comboBox;
    private static volatile ComboBox formattedComboBox;
    private static volatile Spinner spinner;
    private static volatile TextField textFieldWithOnAction;

    public static class TestApp extends Application {

        @Override
        public void start(Stage primaryStage) throws Exception {
            Scene scene = new Scene(createControls());
            primaryStage.setScene(scene);
            stage = primaryStage;
            stage.setOnShown(e -> {
                Platform.runLater(() -> {
                    textField.requestFocus();
                    startupLatch.countDown();
                });
            });
            stage.show();
        }

        private void addToGrid(GridPane grid, Parent cell, int col, int row) {
            GridPane.setConstraints(cell, col, row);
            grid.getChildren().add(cell);
        }

        private Parent createControls() {
            var grid = new GridPane();

            textField = new TextField();
            textField.setEditable(true);
            addToGrid(grid, textField, 0, 0);

            formattedTextField = new TextField();
            formattedTextField.setTextFormatter(
                    new TextFormatter<String>(TextFormatter.IDENTITY_STRING_CONVERTER, ""));
            addToGrid(grid, formattedTextField, 1, 0);

            ObservableList<String> list1 = FXCollections.observableArrayList();
            list1.addAll("b", "c", "a");
            comboBox = new ComboBox(list1);
            comboBox.setEditable(true);
            addToGrid(grid, comboBox, 0, 1);

            ObservableList<String> list2 = FXCollections.observableArrayList();
            list2.addAll("one", "two", "three");
            formattedComboBox = new ComboBox(list2);
            formattedComboBox.getEditor().setTextFormatter(
                    new TextFormatter<String>(TextFormatter.IDENTITY_STRING_CONVERTER, ""));
            formattedComboBox.setEditable(true);
            addToGrid(grid, formattedComboBox, 1, 1);

            spinner = new Spinner<Integer>(0, 100, 5);
            spinner.setEditable(true);
            addToGrid(grid, spinner, 0, 3);

            textFieldWithOnAction = new TextField();
            textFieldWithOnAction.setEditable(true);
            textFieldWithOnAction.setOnAction(a -> {
                textFieldActionFired = true;
            });
            addToGrid(grid, textFieldWithOnAction, 0, 4);

            return grid;
        }
    }

    // Reset at the start of each test.
    private static boolean enterReachedScene;
    private static boolean escapeReachedScene;
    private static boolean textFieldActionFired;

    @BeforeAll
    public static void setupOnce() throws Exception {
        Util.launch(startupLatch, TestApp.class);
        stage.getScene().addEventHandler(KeyEvent.KEY_PRESSED, e -> {
            if (e.getCode() == KeyCode.ENTER && !e.isConsumed()) {
                enterReachedScene = true;
            } else if (e.getCode() == KeyCode.ESCAPE && !e.isConsumed()) {
                escapeReachedScene = true;
            }
        });
    }

    @AfterAll
    public static void teardown() {
        Util.shutdown();
    }

    @BeforeEach
    public void testSetup() {
        enterReachedScene = false;
        escapeReachedScene = false;
        textFieldActionFired = false;
    }

    private void setText(TextField field, String text) {
        field.setText(text);
        field.end();
    }

    private String getTextFromControl(Parent control) {
        if (control instanceof TextField) {
            return ((TextField)control).getText();
        } else if (control instanceof ComboBox) {
            return ((ComboBox)control).getEditor().getText();
        } else if (control instanceof Spinner) {
            return ((Spinner)control).getEditor().getText();
        }
        return "";
    }

    private KeyEvent createKeyPressedEvent(KeyCode code, String character) {
        return new KeyEvent(KeyEvent.KEY_PRESSED, character, "", code, false, false, false, false);
    }

    private KeyEvent createKeyTypedEvent(String character) {
        return new KeyEvent(KeyEvent.KEY_TYPED, character, "", KeyCode.UNDEFINED, false, false, false, false);
    }

    private KeyEvent createKeyReleasedEvent(KeyCode code, String character) {
        return new KeyEvent(KeyEvent.KEY_RELEASED, character, "", code, false, false, false, false);
    }

    private void sendEditingEvents(Parent control) {
        String oldText = getTextFromControl(control);
        control.fireEvent(createKeyPressedEvent(KeyCode.DIGIT1, "1"));
        control.fireEvent(createKeyTypedEvent("1"));
        control.fireEvent(createKeyReleasedEvent(KeyCode.DIGIT1, "1"));
        String newText = getTextFromControl(control);
        assertNotEquals(oldText, newText, "Key events did not edit control");
    }

    private void sendEnter(Parent control) {
        control.fireEvent(createKeyPressedEvent(KeyCode.ENTER, "\n"));
        control.fireEvent(createKeyTypedEvent("\n"));
        control.fireEvent(createKeyReleasedEvent(KeyCode.ENTER, "\n"));
    }

    private void sendEscape(Parent control) {
        control.fireEvent(createKeyPressedEvent(KeyCode.ESCAPE, ""));
        control.fireEvent(createKeyTypedEvent(""));
        control.fireEvent(createKeyReleasedEvent(KeyCode.ESCAPE, ""));
    }

    private void sendEscapeToCancel(Parent control) {
        String oldText = getTextFromControl(control);
        sendEscape(control);
        String newText = getTextFromControl(control);
        assertNotEquals(oldText, newText, "ESCAPE did not cancel editing");
    }

    private void sendEscapeNoCancel(Parent control) {
        String oldText = getTextFromControl(control);
        sendEscape(control);
        String newText = getTextFromControl(control);
        assertEquals(oldText, newText, "ESCAPE altered text unexpectedly");
    }

    // Test ENTER after editing. The ENTER press event should reach the scene
    // unconsumed.
    private void editFollowedByEnter(Parent control) {
        sendEditingEvents(control);
        sendEnter(control);
        assertTrue(enterReachedScene, "ENTER was consumed before reaching scene");
    }

    // Test ESCAPE when the control does not have a formatter and has been
    // edited. The ESCAPE should not affect the text and should reach the
    // scene unconsumed.
    private void editFollowedByEscapeNoFormatter(Parent control) {
        sendEditingEvents(control);
        sendEscapeNoCancel(control);
        assertTrue(escapeReachedScene, "ESCAPE was consumed before reaching scene");
    }

    // Test ESCAPE when the control has a formatter and has been edited. The
    // first ESCAPE should cancel editing and be consumed, the second should
    // reach the scene unconsumed.
    private void editFollowedByEscapeWithFormatter(Parent control) {
        // First Escape should cancel editing, second should leave text unchanged.
        sendEditingEvents(control);
        sendEscapeToCancel(control);
        assertFalse(escapeReachedScene, "ESCAPE reached scene instead of canceling edit");
        sendEscapeNoCancel(control);
        assertTrue(escapeReachedScene, "ESCAPE was consumed before reaching scene");
    }

    // Test ESCAPE when the control has not been edited. The text should not change
    // and the event should reach the scene unconsumed.
    private void noEditEscape(Parent control) {
        sendEscapeNoCancel(control);
        assertTrue(escapeReachedScene, "ESCAPE was consumed before reaching scene");
    }

    // For formatted or unformatted controls, Enter should always reach the Scene.
    @Test
    public void enterOnTextField() {
        Util.runAndWait(() -> {
            textField.setText("b");
            editFollowedByEnter(textField);
        });
        assertEquals("1b", textField.getText());
    }

    @Test
    public void enterOnFormattedTextField() {
        Util.runAndWait(() -> {
            formattedTextField.setText("c");
            editFollowedByEnter(formattedTextField);
        });
        assertEquals("c1", formattedTextField.getText());
    }

    @Test
    public void enterOnComboBox() {
        Util.runAndWait(() -> {
            comboBox.setValue("b");
            editFollowedByEnter(comboBox);
        });
        assertEquals("1b", comboBox.getValue());
    }

    @Test
    public void enterOnFormattedComboBox() {
        Util.runAndWait(() -> {
            formattedComboBox.setValue("b");
            editFollowedByEnter(formattedComboBox);
        });
        assertEquals("b1", formattedComboBox.getValue());
    }

    @Test
    public void enterOnSpinner() {
        Util.runAndWait(() -> {
            spinner.getValueFactory().setValue(2);
            editFollowedByEnter(spinner);
        });
        assertEquals(12, spinner.getValue());
    }

    // If a TextField has an OnAction handler ENTER should be consumed.
    @Test
    public void enterOnTextFieldWithOnAction() {
        Util.runAndWait(() -> {
            textFieldWithOnAction.setText("o");
            sendEditingEvents(textFieldWithOnAction);
            sendEnter(textFieldWithOnAction);
        });
        assertEquals("1o", textFieldWithOnAction.getText());
        assertTrue(textFieldActionFired, "ENTER did not fire text field action");
        assertFalse(enterReachedScene, "ENTER reached scene despite OnAction being set");
    }

    // After editing ESCAPE on an unformatted control should have no effect on
    // the text and reach the scene unconsumed.
    @Test
    public void escapeOnTextFieldAfterEditing() {
        Util.runAndWait(() -> {
            textField.setText("d");
            editFollowedByEscapeNoFormatter(textField);
        });
        assertEquals("1d", textField.getText());
    }

    @Test
    public void escapeOnComboBoxAfterEditing() {
        Util.runAndWait(() -> {
            comboBox.setValue("f");
            editFollowedByEscapeNoFormatter(comboBox);
        });
        assertEquals("f", comboBox.getValue());
    }

    @Test
    public void escapeOnSpinnerAfterEditing() {
        Util.runAndWait(() -> {
            spinner.getValueFactory().setValue(30);
            editFollowedByEscapeNoFormatter(spinner);
        });
        assertEquals(30, spinner.getValue());
    }

    // If a control has not been edited ESCAPE should have no effect
    // on the text and reach the scene unconsumed.
    @Test
    public void escapeOnTextFieldNoEditing() {
        Util.runAndWait(() -> {
            textField.setText("d");
            noEditEscape(textField);
        });
        assertEquals("d", textField.getText());
    }

    @Test
    public void escapeOnFormattedTextFieldNoEditing() {
        Util.runAndWait(() -> {
            formattedTextField.setText("g");
            noEditEscape(formattedTextField);
        });
        assertEquals("g", formattedTextField.getText());
    }

    @Test
    public void escapeOnComboBoxNoEditing() {
        Util.runAndWait(() -> {
            comboBox.setValue("f");
            noEditEscape(comboBox);
        });
        assertEquals("f", comboBox.getValue());
    }

    @Test
    public void escapeOnFormattedComboBoxNoEditing() {
        Util.runAndWait(() -> {
            formattedComboBox.setValue("h");
            noEditEscape(formattedComboBox);
        });
        assertEquals("h", formattedComboBox.getValue());
    }

    @Test
    public void escapeOnSpinnerNoEditing() {
        Util.runAndWait(() -> {
            spinner.getValueFactory().setValue(30);
            noEditEscape(spinner);
        });
        assertEquals(30, spinner.getValue());
    }

    // If a formatter is attached the first ESCAPE should cancel editing and
    // the second should reach the Scene unconsumed.
    @Test
    public void escapeOnFormattedTextField() {
        Util.runAndWait(() -> {
            formattedTextField.setText("e");
            editFollowedByEscapeWithFormatter(formattedTextField);
        });
        assertEquals("e", formattedTextField.getText());
    }

    @Test
    public void escapeOnFormattedComboBox() {
        Util.runAndWait(() -> {
            formattedComboBox.setValue("h");
            editFollowedByEscapeWithFormatter(formattedComboBox);
        });
        assertEquals("h", formattedComboBox.getValue());
    }
}

