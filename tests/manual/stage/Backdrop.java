import javafx.application.Application;
import javafx.application.Platform;
import javafx.application.ColorScheme;
import javafx.application.Platform.Preferences;
import javafx.geometry.Pos;
import javafx.geometry.Insets;
import javafx.scene.control.ToolBar;
import javafx.scene.control.Button;
import javafx.scene.control.ChoiceBox;
import javafx.scene.control.Menu;
import javafx.scene.control.MenuItem;
import javafx.scene.control.MenuBar;
import javafx.scene.control.Label;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HeaderBar;
import javafx.scene.paint.Color;
import javafx.scene.layout.Region;
import javafx.scene.Node;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.SceneBackdrop;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import javafx.stage.Window;
import java.util.List;

public class Backdrop extends Application {
    public static void main(String[] args) {
        launch(Backdrop.class, args);
    }

    private String labelForStyle(StageStyle style) {
        String title = "Unknown";
        switch (style) {
            case DECORATED:
                title = "Decorated";
                break;
            case UNDECORATED:
                title = "Undecorated";
                break;
            case TRANSPARENT:
                title = "Transparent";
                break;
            case UNIFIED:
                title = "Unified";
                break;
            case UTILITY:
                title = "Utility";
                break;
            case EXTENDED:
                title = "Extended";
                break;
        }
        return title;
    }

    private enum TestBackdrop {
        DEFAULT("None", null),
        RECTANGLE("Rectangle", new SceneBackdrop()),
        ROUNDED("Rounded", new SceneBackdrop(30.0)),
        ROUNDED_SHADOW("Rounded w/shadow", new SceneBackdrop(30.0, true));

        private String label;
        private SceneBackdrop backdrop;

        TestBackdrop(String label, SceneBackdrop backdrop) {
            this.label = label;
            this.backdrop = backdrop;
        }

        SceneBackdrop getBackdrop() {
            return backdrop;
        }

        public String toString() {
            return label;
        }
    }

    private enum TestFill {
        NONE("None", null),
        TRANSPARENT("Transparent", Color.TRANSPARENT),
        BLUE("Light blue", Color.LIGHTBLUE),
        TRANSLUCENT_RED("Red (50% opaque)", new Color(1.0, 0.0, 0.0, 0.5));

        private String label;
        private Color color;

        TestFill(String label, Color color) {
            this.label = label;
            this.color = color;
        }

        Color getFill() {
            return color;
        }

        public String toString() {
            return label;
        }
    }

    private enum TestColorScheme {
        LIGHT("Light", ColorScheme.LIGHT),
        DARK("Dark", ColorScheme.DARK);

        private String label;
        private ColorScheme scheme;

        TestColorScheme(String label, ColorScheme scheme) {
            this.label = label;
            this.scheme = scheme;
        }

        ColorScheme getColorScheme() {
            return scheme;
        }

        public String toString() {
            return label;
        }
    }

    private MenuItem stageMenuItem(StageStyle style) {
        var item = new MenuItem(labelForStyle(style));
        item.setOnAction(e -> {
            createAndShowStage(style);
        });
        return item;
    }

    private Parent choiceAndLabel(String label, ChoiceBox choice) {
        VBox box = new VBox(new Label(label), choice);
        box.setSpacing(5);
        return box;
    }

    private void buildScene(Stage stage) {

        // The menus and menu bar
        var closeItem = new MenuItem("Close");
        closeItem.setOnAction(e -> {
            stage.close();
        });
        var windowMenu = new Menu("Window", null, closeItem);

        var decoratedItem = stageMenuItem(StageStyle.DECORATED);
        var undecoratedItem = stageMenuItem(StageStyle.UNDECORATED);
        var transparentItem = stageMenuItem(StageStyle.TRANSPARENT);
        var unifiedItem = stageMenuItem(StageStyle.UNIFIED);
        var utilityItem = stageMenuItem(StageStyle.UTILITY);
        var extendedItem = stageMenuItem(StageStyle.EXTENDED);

        var stageCreateMenu = new Menu("Stage", null,
            decoratedItem, undecoratedItem, transparentItem,
            unifiedItem, utilityItem, extendedItem);

        var menuBar = new MenuBar(windowMenu, stageCreateMenu);
        menuBar.setBackground(null);

        // Boxes for choosing backdrop, colors, etc.
        ChoiceBox<TestBackdrop> backdropChoice = new ChoiceBox<>();
        backdropChoice.getItems().setAll(TestBackdrop.values());

        ChoiceBox<TestFill> fillChoice = new ChoiceBox<>();
        fillChoice.getItems().setAll(TestFill.values());

        // Box for choosing color scheme
        ChoiceBox<TestColorScheme> schemeChoice = new ChoiceBox<>();
        schemeChoice.getItems().setAll(TestColorScheme.values());

        // Pull it together
        VBox controls = new VBox(
            choiceAndLabel("Backdrop", backdropChoice),
            choiceAndLabel("Fill color", fillChoice),
            choiceAndLabel("Color scheme", schemeChoice)
        );

        controls.setAlignment(Pos.BASELINE_LEFT);
        controls.setBackground(null);
        controls.setSpacing(10);
        controls.setPadding(new Insets(10, 10, 10, 10));

        var borderPane = new BorderPane();
        borderPane.setBackground(null);
        borderPane.setTop(menuBar);
        borderPane.setCenter(controls);

        Parent root = borderPane;
        if (stage.getStyle() == StageStyle.EXTENDED) {
            var headerBar = new HeaderBar();
            headerBar.setCenter(new Label(stage.getTitle()));
            var box = new VBox(headerBar, borderPane);
            box.setBackground(null);
            root = box;
        }

        Scene scene = new Scene(root, 640, 480, Color.TRANSPARENT);

        backdropChoice.setOnAction(e -> {
            scene.setBackdrop(backdropChoice.getValue().getBackdrop());
        });
        fillChoice.setOnAction(e -> {
            scene.setFill(fillChoice.getValue().getFill());
        });
        schemeChoice.setOnAction(e -> {
            scene.getPreferences().setColorScheme(schemeChoice.getValue().getColorScheme());
        });

        backdropChoice.setValue(TestBackdrop.RECTANGLE);
        fillChoice.setValue(TestFill.TRANSPARENT);
        schemeChoice.setValue(TestColorScheme.LIGHT);

        stage.setScene(scene);
    }

    private void showStage(Stage stage, StageStyle style)
    {
        stage.setTitle(labelForStyle(style));
        stage.initStyle(style);
        buildScene(stage);
        stage.show();
    }

    private void createAndShowStage(StageStyle style)
    {
        Stage stage = new Stage();
        showStage(stage, style);
    }

    @Override
    public void start(Stage stage) {
        // setUserAgentStylesheet("file:////Users/martin/Java/jfx/teststyles.css");
        Scene.setDefaultBackdrop(null);
        showStage(stage, StageStyle.EXTENDED);
    }
}
