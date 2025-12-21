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
import javafx.stage.Stage;
import javafx.stage.Backdrop;
import javafx.stage.StageStyle;
import javafx.stage.Window;
import java.util.List;

public class BackdropTest extends Application {
    public static void main(String[] args) {
        launch(BackdropTest.class, args);
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

    private enum StageStyleChoice {
        DECORATED("Decorated", StageStyle.DECORATED),
        UNDECORATED("Undecorated", StageStyle.UNDECORATED);

        private String label;
        private StageStyle stageStyle;

        StageStyleChoice(String label, StageStyle style) {
            this.label = label;
            this.stageStyle = style;
        }

        public StageStyle getStageStyle() {
            return stageStyle;
        }

        public String toString() {
            return label;
        }
    }

    private enum BackdropChoice {
        DEFAULT("Default", Backdrop.DEFAULT),
        WINDOW("Window", Backdrop.WINDOW),
        TABBED("Tabbed", Backdrop.TABBED),
        TRANSIENT("Transient", Backdrop.TRANSIENT);

        private String label;
        private Backdrop backdrop;

        BackdropChoice(String label, Backdrop backdrop) {
            this.label = label;
            this.backdrop = backdrop;
        }

        public Backdrop getBackdrop() {
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

    private Parent labeledSection(String label, Parent section) {
        VBox box = new VBox(new Label(label), section);
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

        var menuBar = new MenuBar(windowMenu);
        menuBar.setBackground(null);

        // For creating new stages
        ChoiceBox<StageStyleChoice> stageStyleChoice = new ChoiceBox<>();
        stageStyleChoice.getItems().setAll(StageStyleChoice.values());
        stageStyleChoice.setValue(StageStyleChoice.DECORATED);

        ChoiceBox<BackdropChoice> backdropChoice = new ChoiceBox<>();
        backdropChoice.getItems().setAll(BackdropChoice.values());
        backdropChoice.setValue(BackdropChoice.DEFAULT);

        Button createButton = new Button("Create!");
        createButton.setOnAction(e -> {
            createAndShowStage(stageStyleChoice.getValue().getStageStyle(), backdropChoice.getValue().getBackdrop());
        });

        HBox stageCreationControls = new HBox(stageStyleChoice, backdropChoice, createButton);
        stageCreationControls.setBackground(null);
        stageCreationControls.setSpacing(10);

        ChoiceBox<TestFill> fillChoice = new ChoiceBox<>();
        fillChoice.getItems().setAll(TestFill.values());

        // Box for choosing color scheme
        ChoiceBox<TestColorScheme> schemeChoice = new ChoiceBox<>();
        schemeChoice.getItems().setAll(TestColorScheme.values());

        // Pull it together
        VBox controls = new VBox(
            labeledSection("New stage", stageCreationControls),
            labeledSection("Fill color for this stage", fillChoice),
            labeledSection("Color scheme for this stage", schemeChoice)
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

        fillChoice.setOnAction(e -> {
            scene.setFill(fillChoice.getValue().getFill());
        });
        schemeChoice.setOnAction(e -> {
            scene.getPreferences().setColorScheme(schemeChoice.getValue().getColorScheme());
        });

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

    private void showStage(Stage stage, StageStyle style, Backdrop backdrop)
    {
        stage.setTitle(labelForStyle(style));
        stage.initStyle(style);
        stage.initBackdrop(backdrop);
        buildScene(stage);
        stage.show();
    }

    private void createAndShowStage(StageStyle style, Backdrop backdrop)
    {
        Stage stage = new Stage();
        showStage(stage, style, backdrop);
    }

    @Override
    public void start(Stage stage) {
        // setUserAgentStylesheet("file:////Users/martin/Java/jfx/teststyles.css");
        showStage(stage, StageStyle.EXTENDED, Backdrop.WINDOW);
    }
}
