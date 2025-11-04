import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Pos;
import javafx.geometry.Insets;
import javafx.scene.control.ToolBar;
import javafx.scene.control.Button;
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
import javafx.stage.StageStyle;
import javafx.stage.Window;
import java.util.List;

public class BackgroundStyles extends Application {
    public static void main(String[] args) {
        launch(BackgroundStyles.class, args);
    }

    private void buildScene(Stage stage) {

        Button closeButton = new Button("Close");
        closeButton.setOnAction(e -> {
            stage.close();
        });

        Button decorated = new Button("Decorated");
        decorated.setOnAction(e -> {
            createAndShowStage(StageStyle.DECORATED);
        });

        Button transparent = new Button("Transparent");
        transparent.setOnAction(e -> {
            createAndShowStage(StageStyle.TRANSPARENT);
        });

        Button undecorated = new Button("Undecorated");
        undecorated.setOnAction(e -> {
            createAndShowStage(StageStyle.UNDECORATED);
        });

        Button extended = new Button("Extended");
        extended.setOnAction(e -> {
            createAndShowStage(StageStyle.EXTENDED);
        });

        Button opaqueBackground = new Button("Opaque");
        opaqueBackground.setOnAction(e -> {
            stage.setBackgroundStyle(Window.BackgroundStyle.OPAQUE);
        });

        Button transparentBackground = new Button("Transparent");
        transparentBackground.setOnAction(e -> {
            stage.setBackgroundStyle(Window.BackgroundStyle.TRANSPARENT);
        });

        Button translucentBackground = new Button("Translucent");
        translucentBackground.setOnAction(e -> {
            stage.setBackgroundStyle(Window.BackgroundStyle.TRANSLUCENT);
        });

        Button pinkColor = new Button("Pink");
        pinkColor.setOnAction(e -> {
            stage.getScene().setFill(new Color(1.0, 0.0, 0.0, 0.5));
        });

        Button blueColor = new Button("Blue");
        blueColor.setOnAction(e -> {
            stage.getScene().setFill(new Color(0.0, 0.0, 1.0, 1.0));
        });

        Button clearColor = new Button("Clear");
        clearColor.setOnAction(e -> {
            stage.getScene().setFill(Color.TRANSPARENT);
        });

        var stageButtons = new HBox(decorated, transparent, undecorated, extended);
        stageButtons.setBackground(null);
        stageButtons.setPickOnBounds(false);
        stageButtons.setSpacing(10);
        var backgroundButtons = new HBox(opaqueBackground, transparentBackground, translucentBackground);
        backgroundButtons.setBackground(null);
        backgroundButtons.setPickOnBounds(false);
        backgroundButtons.setSpacing(10);
        var colorButtons = new HBox(pinkColor, blueColor, clearColor);
        colorButtons.setBackground(null);
        colorButtons.setPickOnBounds(false);
        colorButtons.setSpacing(10);

        VBox buttons = new VBox(
            new Label("Stage styles"),
            stageButtons,
            new Label("Background styles"),
            backgroundButtons,
            new Label("Colors"),
            colorButtons,
            new Label("Misc"),
            closeButton
        );

        buttons.setAlignment(Pos.BASELINE_LEFT);
        buttons.setPickOnBounds(false);
        buttons.setBackground(null);
        buttons.setSpacing(10);
        buttons.setPadding(new Insets(10, 10, 10, 10));

        var borderPane = new BorderPane();
        borderPane.setBackground(null);
        borderPane.setCenter(buttons);

        System.out.println(stage.getStyle());
        if (stage.getStyle() == StageStyle.EXTENDED) {
            var headerBar = new HeaderBar();
            borderPane.setTop(headerBar);
        }

        Scene scene = new Scene(borderPane, 640, 480, Color.TRANSPARENT);
        stage.setScene(scene);
    }

    private void showStage(Stage stage, StageStyle style)
    {
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
        }
        stage.setTitle(title);
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
        StageStyle style = StageStyle.EXTENDED;
        final Parameters params = getParameters();
        final List<String> unnamed = params.getUnnamed();
        if (!unnamed.isEmpty()) {
            String p = unnamed.get(0);
            if (p.equals("undecorated"))
                style = StageStyle.UNDECORATED;
            else if (p.equals("transparent"))
                style = StageStyle.TRANSPARENT;
            else if (p.equals("unified"))
                style = StageStyle.UNIFIED;
            else if (p.equals("decorated"))
                style = StageStyle.DECORATED;
            else if (p.equals("extended"))
                style = StageStyle.EXTENDED;
            else
                System.out.println("No idea what " + p + " means");
        }
        showStage(stage, style);
    }
}
