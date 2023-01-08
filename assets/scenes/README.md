# 场景配置文件说明

## 简介

场景配置文件（示例见`base.xml`）是一个xml文件，文件格式类似渲染器[mitsuba](https://mitsuba2.readthedocs.io/en/latest/src/getting_started/file_format.html)中使用的配置xml文件。

## 文件格式

整个场景`scene`内会定义三大类元素：相机`camera`、灯光`envmap`和物体`mesh`。相机和灯光是一个场景中必须的，所以代码中有默认参数，即使配置文件中没有定义也会有默认值。

配置文件的基本格式为：
```
<type name="xxx" value="xxx"/>
```
其中type为变量类型，name为变量名，value为值

例如：

```
<float name="radius" value="0.5"/>
```

表示float变量radius，值为0.5。

### 相机`camera`

| 变量名       | 类型      | 描述                      |
| ------------ | --------- | ------------------------- |
| -            | transform | 相机的位姿，详见transform |
| fov          | float     | 未实现                    |
| focal_length | float     | 焦距，未实现              |

### 灯光`envmap`

预定义环境光类型为`type=hdr`，需要可以实现别的

| 变量名 | 类型    | 描述                  |
| ------ | ------- | --------------------- |
| -      | texture | 环境光，通常是hdr图片 |

###物体`mesh`

mesh可以读入`.obj`格式文件，也可以使用参数化的预定义曲面`Sphere`或者`Cube`，该功能通过编辑mesh变量的属性`type`确定，并提供不同的参数，例如：

```
<mesh type="obj">
	<string name="filename" value="../../meshes/plane.obj"/>
</mesh>

<mesh type="Sphere">
	<vec3 name="center" value="0.0 0.0 0.0"/>
	<float name="radius" value="0.5"/>
</mesh>
```

为两种不同类型的物体读入。以下表示不同type的mesh所需的不同参数变量：

| type   | 变量名   | 类型   | 描述                     |
| ------ | -------- | ------ | ------------------------ |
| obj    | filename | string | .obj文件的路径           |
| Sphere | center   | vec3   | 球的中心位置             |
|        | radius   | float  | 球的半径                 |
| Cube   | center   | vec3   | 正方体的中心位置，未实现 |
|        | size     | float  | 正方体的大小，未实现     |

以下是定义物体所需的其他统一的变量：

| 变量名 | 类型      | 描述                     |
| ------ | --------- | ------------------------ |
| -      | transform | 物体导入后需要进行的变换 |
| -      | material  | 物体的材质，详见material |

### 变换`transform`

变换矩阵通常是一个作用在齐次坐标的4x4矩阵，通常包含旋转和平移两部分。以下是不同type的transform所需不同的参数变量：

| type       | 变量名      | 类型  | 描述                   |
| ---------- | ----------- | ----- | ---------------------- |
| lookat     | eye         | vec3  |                        |
|            | center      | vec3  |                        |
|            | up          | vec3  |                        |
| translate  | translation | vec3  | 平移向量               |
| axis-angle | axis        | vec3  | 绕轴旋转的轴，未实现   |
|            | angle       | float | 绕轴旋转的角度，未实现 |
| mat4       | matrix      | mat4  | 4x4的完整矩阵，未实现  |

### 材质`material`

材质的属性可以通过定义material的`type`属性，可选的有`lambertian`、`specular`、`transmissive`和`principled`，分别对应代码最基础的四种属性。

| 变量名       | 类型               | 描述                                                   |
| ------------ | ------------------ | ------------------------------------------------------ |
| albedo       | texture或rgb或rgba | 物体的纹理albedo，可以是单色rgb/rgba，也可以是纹理贴图 |
| albedo_color | vec3               | 反射颜色，未实现                                       |

###纹理`texture`

目前只实现了导入图片。checker只是为了举例。

| type    | 变量名   | 类型   | 描述                       |
| ------- | -------- | ------ | -------------------------- |
| bitmap  | filename | string | 图片路径                   |
| checker | color1   | vec3   | 棋盘格的一种颜色，未实现   |
|         | color2   | vec3   | 棋盘格的另一种颜色，未实现 |



`base.xml`可以更方便你理解该文件：

```
<?xml version="1.0" encoding="utf-8"?>
<scene>

	<camera>
		<transform type="lookat">
			<vec3 name="eye" value="2.0 1.0 3.0"/>
			<vec3 name="center" value="0.0 0.0 0.0"/>
			<vec3 name="up" value="0.0 1.0 0.0"/>
			<!--<lookat eye="2.0 1.0 3.0" center="0.0 0.0 0.0" up="0.0 1.0 0.0"/>-->
		</transform>
	</camera>

	<envmap type="hdr">
		<string name="filename" value="../../textures/envmap_clouds_4k.hdr"/>
	</envmap>

	<mesh type="obj">
		<string name="filename" value="../../meshes/plane.obj"/>

		<material type="lambertian">
			<rgb name="albedo" value="0.8 0.8 0.8"/>
		</material>
	</mesh>

	<mesh type="Sphere">
		<vec3 name="center" value="0.0 0.0 0.0"/>
		<float name="radius" value="0.5"/>

		<transform type="translate">
			<vec3 name="translation" value="0.0 0.5 0.0"/>
		</transform>

		<material type="lambertian">
			<texture name="albedo">
				<string name="filename" value="../../textures/earth.jpg"/>
			</texture>
		</material>
	</mesh>

</scene>
```
