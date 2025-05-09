/*
 * Copyright (c) 2011, 2024, Oracle and/or its affiliates. All rights reserved.
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

package test.javafx.binding.expression;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import javafx.beans.binding.DoubleBinding;
import javafx.beans.binding.FloatBinding;
import javafx.beans.binding.IntegerBinding;
import javafx.beans.binding.IntegerExpression;
import javafx.beans.binding.LongBinding;
import javafx.beans.binding.ObjectExpression;
import javafx.beans.property.IntegerProperty;
import javafx.beans.property.SimpleIntegerProperty;
import javafx.beans.value.ObservableIntegerValueStub;
import javafx.beans.value.ObservableValue;
import javafx.beans.value.ObservableValueStub;
import javafx.collections.FXCollections;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.assertThrows;

public class IntegerExpressionTest {

    private static final float EPSILON = 1e-6f;

    private int data;
    private IntegerProperty op1;
    private double double1;
    private float float1;
    private long long1;
    private int integer1;
    private short short1;
    private byte byte1;

    @BeforeEach
    public void setUp() {
        data = 34258;
        op1 = new SimpleIntegerProperty(data);
        double1 = -234.234;
        float1 = 111.9f;
        long1 = 2009234L;
        integer1 = -234734;
        short1 = 9824;
        byte1 = -123;
    }

    @Test
    public void testGetters() {
        assertEquals(data, op1.doubleValue(), EPSILON);
        assertEquals(data, op1.floatValue(), EPSILON);
        assertEquals(data, op1.longValue());
        assertEquals(data, op1.intValue());
    }

    @Test
    public void testNegation() {
        final IntegerBinding binding1 = op1.negate();
        assertEquals(-data, binding1.intValue());
    }

    @Test
    public void testPlus() {
        final DoubleBinding binding1 = op1.add(double1);
        assertEquals(data + double1, binding1.doubleValue(), EPSILON);

        final FloatBinding binding2 = op1.add(float1);
        assertEquals(data + float1, binding2.floatValue(), EPSILON);

        final LongBinding binding3 = op1.add(long1);
        assertEquals(data + long1, binding3.longValue());

        final IntegerBinding binding4 = op1.add(integer1);
        assertEquals(data + integer1, binding4.intValue());

        final IntegerBinding binding5 = op1.add(short1);
        assertEquals(data + short1, binding5.intValue());

        final IntegerBinding binding6 = op1.add(byte1);
        assertEquals(data + byte1, binding6.intValue());
    }

    @Test
    public void testMinus() {
        final DoubleBinding binding1 = op1.subtract(double1);
        assertEquals(data - double1, binding1.doubleValue(), EPSILON);

        final FloatBinding binding2 = op1.subtract(float1);
        assertEquals(data - float1, binding2.floatValue(), EPSILON);

        final LongBinding binding3 = op1.subtract(long1);
        assertEquals(data - long1, binding3.longValue());

        final IntegerBinding binding4 = op1.subtract(integer1);
        assertEquals(data - integer1, binding4.intValue());

        final IntegerBinding binding5 = op1.subtract(short1);
        assertEquals(data - short1, binding5.intValue());

        final IntegerBinding binding6 = op1.subtract(byte1);
        assertEquals(data - byte1, binding6.intValue());
    }

    @Test
    public void testTimes() {
        final DoubleBinding binding1 = op1.multiply(double1);
        assertEquals(data * double1, binding1.doubleValue(), EPSILON);

        final FloatBinding binding2 = op1.multiply(float1);
        assertEquals(data * float1, binding2.floatValue(), EPSILON);

        final LongBinding binding3 = op1.multiply(long1);
        assertEquals(data * long1, binding3.longValue());

        final IntegerBinding binding4 = op1.multiply(integer1);
        assertEquals(data * integer1, binding4.intValue());

        final IntegerBinding binding5 = op1.multiply(short1);
        assertEquals(data * short1, binding5.intValue());

        final IntegerBinding binding6 = op1.multiply(byte1);
        assertEquals(data * byte1, binding6.intValue());
    }

    @Test
    public void testDividedBy() {
        final DoubleBinding binding1 = op1.divide(double1);
        assertEquals(data / double1, binding1.doubleValue(), EPSILON);

        final FloatBinding binding2 = op1.divide(float1);
        assertEquals(data / float1, binding2.floatValue(), EPSILON);

        final LongBinding binding3 = op1.divide(long1);
        assertEquals(data / long1, binding3.longValue());

        final IntegerBinding binding4 = op1.divide(integer1);
        assertEquals(data / integer1, binding4.intValue());

        final IntegerBinding binding5 = op1.divide(short1);
        assertEquals(data / short1, binding5.intValue());

        final IntegerBinding binding6 = op1.divide(byte1);
        assertEquals(data / byte1, binding6.intValue());
    }

    @Test
    public void testFactory() {
        final ObservableIntegerValueStub valueModel = new ObservableIntegerValueStub();
        final IntegerExpression exp = IntegerExpression.integerExpression(valueModel);

        assertTrue(exp instanceof IntegerBinding);
        assertEquals(FXCollections.singletonObservableList(valueModel), ((IntegerBinding)exp).getDependencies());

        assertEquals(0, exp.intValue());
        valueModel.set(data);
        assertEquals(data, exp.intValue());
        valueModel.set(integer1);
        assertEquals(integer1, exp.intValue());

        // make sure we do not create unnecessary bindings
        assertEquals(op1, IntegerExpression.integerExpression(op1));
    }

    @Test
    public void testAsObject() {
        final ObservableIntegerValueStub valueModel = new ObservableIntegerValueStub();
        final ObjectExpression<Integer> exp = IntegerExpression.integerExpression(valueModel).asObject();

        assertEquals(Integer.valueOf(0), exp.getValue());
        valueModel.set(data);
        assertEquals(Integer.valueOf(data), exp.getValue());
        valueModel.set(integer1);
        assertEquals(Integer.valueOf(integer1), exp.getValue());
    }

    @Test
    public void testObjectToInteger() {
        final ObservableValueStub<Integer> valueModel = new ObservableValueStub<>();
        final IntegerExpression exp = IntegerExpression.integerExpression(valueModel);

        assertTrue(exp instanceof IntegerBinding);
        assertEquals(FXCollections.singletonObservableList(valueModel), ((IntegerBinding)exp).getDependencies());

        assertEquals(0, exp.intValue());
        valueModel.set(data);
        assertEquals(data, exp.intValue());
        valueModel.set(integer1);
        assertEquals(integer1, exp.intValue());

        // make sure we do not create unnecessary bindings
        assertEquals(op1, IntegerExpression.integerExpression((ObservableValue)op1));
    }

    @Test
    public void testFactory_Null() {
        assertThrows(NullPointerException.class, () -> {
            IntegerExpression.integerExpression(null);
        });
    }

}
