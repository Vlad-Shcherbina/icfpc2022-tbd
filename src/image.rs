use crate::basic::{Color, Shape};

#[derive(Clone, PartialEq, Eq, Debug)]
pub struct Image {
    pub width: i32,
    pub height: i32,
    data: Vec<Color>,
}

impl Image {
    pub fn new(width: i32, height: i32, color: Color) -> Image {
        Image {
            width,
            height,
            data: vec![color; (width * height) as usize],
        }
    }

    pub fn load(path: &std::path::Path) -> Image {
        let img = image::open(crate::util::project_path(path)).unwrap().to_rgba8();
        Image::from_raw_image(&img)
    }

    pub fn save(&self, path: &std::path::Path) {
        self.to_raw_image().save(path).unwrap();
    }

    pub fn to_raw_image(&self) -> image::RgbaImage {
        let mut img = image::RgbaImage::new(self.width as u32, self.height as u32);
        for (x, y, pixel) in img.enumerate_pixels_mut() {
            let c = self.get_pixel(x as i32, self.height - 1 - y as i32);
            pixel.0 = c.0;
        }
        img
    }

    pub fn from_raw_image(img: &image::RgbaImage) -> Self {
        let mut res = Image::new(img.width() as i32, img.height() as i32, Color::default());
        for (x, y, pixel) in img.enumerate_pixels() {
            res.set_pixel(
                x as i32, res.height - 1 - y as i32,
                Color(pixel.0));
        }
        res
    }

    pub fn set_pixel(&mut self, x: i32, y: i32, color: Color) {
        // TODO: range checks maybe
        self.data[(y * self.width + x) as usize] = color;
    }

    pub fn get_pixel(&self, x: i32, y: i32) -> Color {
        self.data[(y * self.width + x) as usize]
    }

    pub fn fill_rect(&mut self, shape: Shape, color: Color) {
        for y in shape.y1..shape.y2 {
            for x in shape.x1..shape.x2 {
                self.set_pixel(x, y, color);
            }
        }
    }
}
